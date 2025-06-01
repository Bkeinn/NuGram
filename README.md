Sure, here is an updated version of your README:

```markdown
# NuGram

This repository contains two C++ programs for processing text and creating datasets based on n-grams.

## Nugram

This program reads text from standard input and outputs a CSV file showing the frequency of each n-gram in the text.

### Compilation

```sh
git clone https://github.com/Bkeinn/NuGram.git
cd NuGram
g++ -O3 main.cpp -o nugram
```

### Usage

```sh
./nugram < textfile.txt
```

To capture the output to a file:

```sh
./nugram < textfile.txt > result.csv
```

## Dataset Creator

This program takes the output of Nugram and a text file, then creates an HDF5 dataset where each token is surrounded by the tokens appearing in the original text.

### Compilation

```sh
git clone https://github.com/Bkeinn/NuGram.git
cd NuGram
g++ -std=c++11 -O2 -o tokenize_and_build dataset_creator.cpp -lhdf5_cpp -lhdf5
```

### Usage

```sh
./tokenize_and_build <ngrams_file> <input_text_file> <output_hdf5_file>
```

### Dataset Structure

The dataset has two main components:
- `/tokens/id_to_ngram`: A 1D dataset assigning variable-length strings to a token ID.
- `/contexts/ctx_<ID>`: An expandable Nx8 INT32 dataset per token ID, collecting all surrounding tokens.

## Parameters

You can modify the following parameters at the beginning of the `main` function in `main.cpp`:

```cpp
const int min_ascii = 32;
const int max_ascii = 126;
const int deapth = 5;
const int output_deapth = 253 * 2;
const long long purge_threshold = 1'000;
const long long purge_interval = 1'000'000;
```

## Output

The output of Nugram is sorted by the third column, which is `(length - 1) * frequency`. This is useful for further processing.
```
