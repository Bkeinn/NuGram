//The paramters that are easily changeable, are the
//deapth the algorithem is searching the n-grams from
//and how big the scope for the searchable characters is

#include <bits/stdc++.h>

int width;
int min_ascii;
int max_ascii;

struct Tree {
  int passes;
  std::vector<Tree*> next;

  Tree(int width) : passes(0), next(width ,nullptr) {};
};

Tree* add(Tree* root, char character) {
  if (root->next[character] == nullptr) {
    root->next[character] = new Tree(width);
  }
  root->next[character]->passes++;
  return root->next[character];
}

void adder(std::vector<char> vec, Tree* root){
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
  // !!!! This deapth is changeable and gives the maximum of n-gram deapth it is searching for
  int deapth = 14;
  width = 128;
  min_ascii = 32;
  max_ascii = 126;
   
  char ch;
  std::vector<char> context(deapth, 0);

  Tree* ngram = new Tree(width);

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

  //printtree(ngram, "'");
  
  return 0;
}
