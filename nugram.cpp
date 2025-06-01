#include <bits/stdc++.h>

const int min_ascii = 32;
const int max_ascii = 126;
const int deapth = 5;
const int output_deapth = 253 * 2;
const long long purge_threshold = 1'000; // Threshold for purging nodes
const long long purge_interval = 1'000'000; // Interval for triggering purge

struct Data {
    std::string data;
    long long count;
    long long value;

    Data(std::string d, long long c) : data(d), count(c), value((d.size() - 1) * c) {};
};

std::vector<Data> saver;

struct Tree {
    int passes;
    std::array<Tree*, 256> next;

    Tree() : passes(0) {
        next.fill(nullptr); // Initialize all pointers to nullptr
    }

    void purge(int current_depth, int max_depth) {
        for (int i = 0; i < 256; i++) {
            if (next[i] != nullptr) {
                if (current_depth < max_depth) {
                    next[i]->purge(current_depth + 1, max_depth);
                }
                if (next[i]->passes < purge_threshold) {
                    delete next[i];
                    next[i] = nullptr;
                }
            }
        }
    }
};

Tree* add(Tree* root, char character) {    
    if (root->next[character] == nullptr) {
        root->next[character] = new Tree();
    }
    root->next[character]->passes++;


    return root->next[character];
}

void adder(std::array<char, deapth>& vec, Tree* root) {
    if (vec[0] == 0) {
        return;
    }

    Tree* base = root;
    for (int i = 0; i < vec.size(); i++) {
        base = add(base, static_cast<unsigned char>(vec[i]));
    }
}

void print_tree(Tree* node, const std::string& s) {
    std::cout << s << "','" << node->passes << ',' << (s.size() - 1) * node->passes << "\n";
    for (int i = min_ascii; i <= max_ascii; ++i) {
        if (node->next[i] != nullptr) {
            print_tree(node->next[i], s + static_cast<char>(i));
        }
    }
}

void add_to_saver(Tree* root, std::string s) {
    saver.emplace_back(Data(s, root->passes));
    for (int i = min_ascii; i <= max_ascii; ++i) {
        if (root->next[i] != nullptr) {
            add_to_saver(root->next[i], s + static_cast<char>(i));
        }
    }
}

void saver_exporter() {
    struct {
        bool operator()(const Data a, const Data b) const {
            return a.value > b.value;
        }
    } custom;
    std::sort(saver.begin(), saver.end(), custom);
    // for (const Data& d : saver) {
    //     std::cout << '\'' << d.data << "'," << d.count << ',' << d.value << '\n';
    // }
    std::cout << ',';
    for(int i = 0;i < output_deapth; i++) {
        std::cout << saver[i].data << ',';
    }
    std::cout << std::endl;
}

int main() {
    char ch;
    std::array<char, deapth> context{};
    // std::freopen("/home/heimchen/Documents/Personal/MolRecap.typ", "r", stdin);

    Tree* ngram = new Tree();

    int purge_counter = 0;

    while (std::cin.get(ch)) {
        if (int(ch) >= min_ascii && int(ch) <= max_ascii && ch != '\'' && ch != ',') {
            std::rotate(context.begin(), context.begin() + 1, context.end());
            context[deapth - 1] = ch;
            adder(context, ngram);
            purge_counter++;
            if(purge_counter % purge_interval == 0) ngram->purge(0, deapth);
        }
        if (int(ch) == 8) {
            break;
        }
    }

    for (int i = min_ascii; i <= max_ascii; ++i) {
        if (ngram->next[i]) {
            std::string s;
            add_to_saver(ngram->next[i], s.assign(1, static_cast<char>(i)));
        }
    }
    saver_exporter();

    return 0;
}
