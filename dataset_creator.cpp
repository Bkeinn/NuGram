// contexts_stream.cpp
//
// Usage:
//   ./tokenize_and_build <ngrams_file> <input_text_file> <output_hdf5_file>
//
// Changes from previous version:
//  - Uses <bits/stdc++.h> for STL convenience.
//  - Memory‐maps the input text (up to ~5 GiB) via POSIX mmap (no full buffering).
//  - Treats any byte ≥ 128 as “non‐ASCII” (i.e., no n‐gram can match there).
//  - Parses n‐grams from a file where the line starts and ends with commas, e.g. ",ng1,ng2,ng3,".
//    Empty entries (before the first comma or after the last) are skipped.
//
// Requires linking to HDF5 C++: -lhdf5_cpp -lhdf5
// Compile example:
//   g++ -std=c++11 -O2 -o tokenize_and_build contexts_stream.cpp -lhdf5_cpp -lhdf5

#include <bits/stdc++.h>
#include <H5Cpp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace H5;
using namespace std;

// ------------------------
// 1) Trie for ASCII n-grams
// ------------------------

struct TrieNode {
    // children pointers for ASCII [0..127]
    TrieNode* child[128];
    int token_id; // -1 if non-terminal; otherwise ID

    TrieNode() : token_id(-1) {
        memset(child, 0, sizeof(child));
    }
};

class Trie {
public:
    Trie() {
        root = new TrieNode();
    }
    ~Trie() {
        free_subtree(root);
    }

    // Insert ASCII-only string s with ID = id
    void insert(const string& s, int id) {
        TrieNode* cur = root;
        for (unsigned char uc : s) {
            if (uc >= 128) {
                cerr << "Error: non-ASCII in n-gram \"" << s << "\"\n";
                exit(EXIT_FAILURE);
            }
            if (!cur->child[uc]) {
                cur->child[uc] = new TrieNode();
            }
            cur = cur->child[uc];
        }
        cur->token_id = id;
    }

    // Given a pointer `data + pos` and total length `N`, return (token_id, match_len)
    // for the longest-matching n-gram starting at pos.  If none, returns (-1, 0).
    pair<int, size_t> longest_match(const char* data, size_t pos, size_t N) const {
        TrieNode* cur = root;
        int best_id = -1;
        size_t best_len = 0;

        for (size_t k = pos; k < N; k++) {
            unsigned char uc = static_cast<unsigned char>(data[k]);
            if (uc >= 128 || !cur->child[uc]) {
                break;
            }
            cur = cur->child[uc];
            if (cur->token_id >= 0) {
                best_id = cur->token_id;
                best_len = (k - pos) + 1;
            }
        }
        return {best_id, best_len};
    }

private:
    TrieNode* root;

    void free_subtree(TrieNode* node) {
        if (!node) return;
        for (int i = 0; i < 128; i++) {
            if (node->child[i]) free_subtree(node->child[i]);
        }
        delete node;
    }
};

// ------------------------
// 2) HDF5 Writer: one string dataset + M context datasets (0×8 → extendable)
// ------------------------

struct HDF5Writer {
    HDF5Writer(const string& filename, const vector<string>& id_to_ngram)
        : file(filename, H5F_ACC_TRUNC)
    {
        size_t M = id_to_ngram.size();

        // Create "/tokens" group and 1D var-len string dataset "id_to_ngram"
        Group tokens_grp = file.createGroup("/tokens");
        StrType strdatatype(PredType::C_S1, H5T_VARIABLE);
        hsize_t dims[1] = {static_cast<hsize_t>(M)};
        DataSpace dsp(1, dims);
        DataSet ds = tokens_grp.createDataSet("id_to_ngram", strdatatype, dsp);

        vector<const char*> cstrs(M);
        for (size_t i = 0; i < M; i++) {
            cstrs[i] = id_to_ngram[i].c_str();
        }
        ds.write(cstrs.data(), strdatatype);

        // Create "/contexts" group and, for each ID, an empty (0×8) dataset
        contexts_grp = file.createGroup("/contexts");
        datasets.reserve(M);
        for (size_t i = 0; i < M; i++) {
            string dname = "ctx_" + to_string(i);
            hsize_t init_dims[2] = {0, 8};
            hsize_t max_dims[2]  = {H5S_UNLIMITED, 8};
            DataSpace space(2, init_dims, max_dims);

            // Chunk shape (e.g., 1024×8)
            hsize_t chunk_dims[2] = {1024, 8};
            DSetCreatPropList plist;
            plist.setChunk(2, chunk_dims);

            DataSet ctx_ds = contexts_grp.createDataSet(
                dname, PredType::NATIVE_INT32, space, plist
            );
            datasets.push_back(ctx_ds);
        }
    }

    // Append one row (8 ints) to dataset "ctx_<token_id>"
    void append_context_row(int token_id, const array<int,8>& row) {
        DataSet& ds = datasets[token_id];
        DataSpace filespace = ds.getSpace();
        hsize_t cur_dims[2];
        filespace.getSimpleExtentDims(cur_dims);

        // Extend first dimension by 1
        hsize_t new_dims[2] = {cur_dims[0] + 1, 8};
        ds.extend(new_dims);

        DataSpace newspace = ds.getSpace();
        hsize_t offset[2] = {cur_dims[0], 0};
        hsize_t count[2]  = {1, 8};
        newspace.selectHyperslab(H5S_SELECT_SET, count, offset);

        hsize_t mem_dims[2] = {1, 8};
        DataSpace memspace(2, mem_dims);

        ds.write(row.data(), PredType::NATIVE_INT32, memspace, newspace);
    }

private:
    H5File file;
    Group contexts_grp;
    vector<DataSet> datasets;
};

// ------------------------
// 3) Main: parse n-grams, mmap input, tokenize & write contexts
// ------------------------

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0]
             << " <ngrams_file> <input_text_file> <output_hdf5_file>\n";
        return EXIT_FAILURE;
    }
    string ngrams_path   = argv[1];
    string input_path    = argv[2];
    string output_h5path = argv[3];

    //
    // 3a) Read n-grams from a single line that starts and ends with commas.
    //      Example: ",ng1,ng2,ng3,"
    //
    ifstream nfin(ngrams_path);
    if (!nfin) {
        cerr << "Error: cannot open ngrams file \"" << ngrams_path << "\"\n";
        return EXIT_FAILURE;
    }
    string line;
    getline(nfin, line);
    nfin.close();

    vector<string> id_to_ngram;
    {
        // Split on ',', ignore empty tokens
        size_t start = 0, len = line.size();
        while (start < len) {
            size_t comma = line.find(',', start);
            if (comma == string::npos) comma = len;
            string token = line.substr(start, comma - start);
            if (!token.empty()) {
                id_to_ngram.push_back(token);
            }
            start = comma + 1;
        }
    }
    size_t M = id_to_ngram.size();
    if (M == 0) {
        cerr << "Error: no n-grams found in \"" << ngrams_path << "\"\n";
        return EXIT_FAILURE;
    }

    //
    // 3b) Build Trie over n-grams
    //
    Trie trie;
    for (size_t i = 0; i < M; i++) {
        trie.insert(id_to_ngram[i], static_cast<int>(i));
    }

    //
    // 3c) Memory-map the input text (possibly up to ~5 GiB)
    //
    int fd = open(input_path.c_str(), O_RDONLY);
    if (fd < 0) {
        perror("Error opening input_text_file");
        return EXIT_FAILURE;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    size_t N = static_cast<size_t>(st.st_size);
    if (N == 0) {
        cerr << "Error: input text file is empty\n";
        close(fd);
        return EXIT_FAILURE;
    }
    // PROT_READ, MAP_PRIVATE (no modifications), map the whole file.
    void* mmap_ptr = mmap(nullptr, N, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmap_ptr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    const char* data = static_cast<const char*>(mmap_ptr);
    // We can close the FD but keep the mapping alive
    close(fd);

    //
    // 4) Create HDF5 structures
    //
    HDF5Writer writer(output_h5path, id_to_ngram);

    //
    // 5) Tokenization + context extraction
    //
    // We replicate the “for each character index i in [0..N-1], try to match 9 tokens greedily” logic.
    // Use a small vector<int> of size up to 9 to hold token IDs, then slide.
    //

    vector<int> buffer_ids;
    buffer_ids.reserve(9);

    for (size_t i = 0; i < N; i++) {
        buffer_ids.clear();
        size_t j = i;

        // 5.1) Attempt to collect exactly 9 tokens starting at text position i
        bool ok = true;
        while (buffer_ids.size() < 9 && j < N) {
            unsigned char uc = static_cast<unsigned char>(data[j]);
            if (uc >= 128) {
                ok = false;
                break;
            }
            auto match = trie.longest_match(data, j, N);
            int  tok_id = match.first;
            size_t tok_len = match.second;
            if (tok_id < 0) {
                ok = false;
                break;
            }
            buffer_ids.push_back(tok_id);
            j += tok_len;
        }
        if (!ok || buffer_ids.size() < 9) {
            continue; // cannot form 9 tokens from i
        }

        // 5.2) We have 9 token IDs in buffer_ids[0..8], and j is after the 9th token.
        auto write_middle_row = [&](const vector<int>& buf) {
            array<int,8> row;
            for (int k = 0; k < 4; k++) {
                row[k]   = buf[k];
                row[k+4] = buf[k+5];
            }
            int mid_id = buf[4];
            writer.append_context_row(mid_id, row);
        };

        // Write the first context for this 9-token block
        write_middle_row(buffer_ids);

        // 5.3) Slide one token at a time
        vector<int> slide_buf = buffer_ids;
        size_t slide_j = j;
        while (slide_j < N) {
            unsigned char uc = static_cast<unsigned char>(data[slide_j]);
            if (uc >= 128) break;
            auto match = trie.longest_match(data, slide_j, N);
            int  tok_id = match.first;
            size_t tok_len = match.second;
            if (tok_id < 0) break;

            // Slide left by one
            for (int k = 0; k < 8; k++) {
                slide_buf[k] = slide_buf[k+1];
            }
            slide_buf[8] = tok_id;
            slide_j += tok_len;

            write_middle_row(slide_buf);
        }

        // Move to next i; buffer_ids cleared by next iteration
    }

    // Cleanup: unmap
    if (munmap((void*)data, N) < 0) {
        perror("munmap");
    }

    cout << "Finished writing contexts to \"" << output_h5path << "\"\n";
    return EXIT_SUCCESS;
}
