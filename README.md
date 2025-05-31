# What is it
This code, takes in text through the unix pipe and spits out 
an csv that shows how often each n-gram appeard in the text. 

# How to use
Compile the program with
```
https://github.com/Bkeinn/NuGram.git
cd NuGram
g++ -O3 main.cpp -o nugram
```
The you have to execute the program and pipe a file to it
```
./nugram < textfile.txt  
```
This will print the csv file into the command line, to capture it
```
./nugram < testfile.txt > result.csv
```

# Speed

## 50.000 Zeichen 14 deep n-gram
g++ -O3 main.cpp -o nugram
```
Executed in  338.88 millis    fish           external
   usr time  214.35 millis    0.00 millis  214.35 millis
   sys time  124.61 millis    4.04 millis  120.57 millis
```
## Result
- The first column are the n-grams(which at least in my version
contain white spaces).
- Second column how often these n-grams appeard
in the text.
- Third (length - 1) * how_often

> The output is allready sorted by the last collumn
### For furhter use
If you set the output_deapth parameter and uncomment/comment these lines
```cpp
  // for(int i = 0;i < output_deapth - 1; i++) {
  //   std::cout << saver[i].data << ',';
  // }
  // std::cout << saver[output_deapth - 1].data << std::endl;
```

and comment these lines above
```cpp
  for(const Data& d : saver){
    std::cout << '\'' << d.data << "'," << d.count << ',' << d.value << '\n';
  }
```
It outputs a sorted list of the most common ngrams, sorted by the last collumn
this, is very usefull for further use

# Modify
You are able to modify the program to your liking but the main
handles one could tune are the deapth to witch n-grams are searched for and
witch characters the program is searching for. 
The are all defined at the beginning of the main funktion.
```cpp
const int min_ascii = 32;
const int max_ascii = 126;
const int width = 128 - min_ascii;
const int deapth = 5;
const int output_deapth = 253*2;
```
