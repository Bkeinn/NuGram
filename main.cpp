//The paramters that are easily changeable, are the
//deapth the algorithem is searching the n-grams from
//and how big the scope for the searchable characters is

#include <bits/stdc++.h>

// !!!! This deapth is changeable and gives the maximum of n-gram deapth it is searching for
const int min_ascii = 32;
const int max_ascii = 126;
const int width = 128 - min_ascii;
const int deapth = 5;

struct Tree {
  int passes;
  std::array<Tree*, 256> next;

  Tree() : passes(0) {};
};

Tree* add(Tree* root, char character) {
  if (root->next[character - min_ascii] == nullptr) {
    root->next[character - min_ascii] = new Tree();
    // std::cout << character << std::endl;
  }
  root->next[character - min_ascii]->passes++;
  return root->next[character - min_ascii];
}

void adder(std::array<char, deapth> vec, Tree* root){
  if (vec[0] == 0){
    return;
  }

  Tree* base = root;
  for(int i = 0; i < vec.size(); i++){
    base = add(base, vec[i]);
  }
}

void printtree(Tree* root, std::string s){
  std::cout << s << "'," << root->passes << "," << (s.size() - 2) * root->passes << ",\n";
  for(int i = 0; i < root->next.size() - 1; i++){
    if(root->next[i] != nullptr){
      printtree(root->next[i], s + char(i));
    }
  }
}

int main(){
  char ch;
  std::array<char,deapth> context;

  Tree* ngram = new Tree();

  while (std::cin.get(ch)){
      if(int(ch) >= min_ascii && int(ch) <= max_ascii){
        std::rotate(context.begin(), context.begin() + 1, context.end());
        context[deapth - 1] = ch;
        adder(context, ngram);
      }
      if(int(ch) == 8){
        break;
      }
  }

  printtree(ngram, "'");
  
  return 0;
}
