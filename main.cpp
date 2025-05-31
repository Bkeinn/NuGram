//The paramters that are easily changeable, are the
//deapth the algorithem is searching the n-grams from
//and how big the scope for the searchable characters is

#include <bits/stdc++.h>

// !!!! This deapth is changeable and gives the maximum of n-gram deapth it is searching for
const int min_ascii = 32;
const int max_ascii = 126;
const int width = 128 - min_ascii;
const int deapth = 5;
const int output_deapth = 253*2;

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

  Tree() : passes(0), next{} {};
};

Tree* add(Tree* root, char character) {
  if (root->next[character] == nullptr) {
    root->next[character] = new Tree();
    // std::cout << character << std::endl;
  }
  root->next[character]->passes++;
  return root->next[character];
}

void adder(std::array<char, deapth>& vec, Tree* root){
  if (vec[0] == 0){
    return;
  }

  Tree* base = root;
  for(int i = 0; i < vec.size(); i++){
    base = add(base, vec[i]);
  }
}

void print_tree(Tree* node, const std::string& s) {
  // s starts with one leading apostrophe
  std::cout << s << "','" << node->passes
            << ',' << (s.size()-1)*node->passes 
            << "\n";
  for (int i = min_ascii; i <= max_ascii; ++i) {
    if (node->next[i] != nullptr) {
      print_tree(node->next[i], s + char(i));
    }
  }
}


void add_to_saver(Tree* root, std::string s){
  saver.emplace_back(Data(s, root->passes));
  for (int i = min_ascii; i <= max_ascii; ++i) {
    if (root->next[i] != nullptr) {
      add_to_saver(root->next[i], s + char(i));
    }
  }
}

void saver_exporter() {
  struct {
    bool operator() (const Data a, const Data b) const {
      return a.value > b.value;
    }
  } custom;
  std::sort(saver.begin(), saver.end(), custom);
  for(const Data& d : saver){
    std::cout << '\'' << d.data << "'," << d.count << ',' << d.value << '\n';
  }
  // for(int i = 0;i < output_deapth - 1; i++) {
  //   std::cout << saver[i].data << ',';
  // }
  // std::cout << saver[output_deapth - 1].data << std::endl;
}

int main(){
  char ch;
  std::array<char,deapth> context{};
  //std::freopen("/home/heimchen/Documents/Personal/MolRecap.typ", "r", stdin);

  Tree* ngram = new Tree();

  while (std::cin.get(ch)){
      if(int(ch) >= min_ascii && int(ch) <= max_ascii && char(ch) != '\'' && char(ch) != ','){ // this caps out ' and , so the csv does not get destroyed
        std::rotate(context.begin(), context.begin() + 1, context.end());
        context[deapth - 1] = ch;
        adder(context, ngram);
      }
      if(int(ch) == 8){
        break;
      }
  }

  for (int i = min_ascii; i <= max_ascii; ++i) {
    if (ngram->next[i]) {
      //std::cout << ngram->next[i]->passes << "\n";
      //print_tree(ngram->next[i], "'" + char(i));
      std::string s;
      add_to_saver(ngram->next[i], s.assign(1,char(i)));
    }
  }

  saver_exporter();
  
  return 0;
}
