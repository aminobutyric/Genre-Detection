#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <ctime> // for sorting stories by load time
#include <algorithm> // for std::sort
#include <unordered_set>
#include <numeric>

using namespace std;


// Function to read CSV file and populate genre-keyword mapping with weights
unordered_map<string, vector<pair<string, int>>> readCSV(const string& csvFilePath) {
    unordered_map<string, vector<pair<string, int>>> genreKeywordMap;

    ifstream file(csvFilePath);
    if (!file.is_open()) {
        throw runtime_error("Error importing genre keywords. Please check keyword files.");
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string keyword;
        int weight;
        char comma; // to handle the comma between keyword and weight
        ss >> keyword >> comma >> weight;

        // You may want to trim whitespace from the keyword
        genreKeywordMap[csvFilePath].push_back({keyword, weight});
    }

    file.close();

    return genreKeywordMap;
}

vector<string> analyzedStoriesList;

/// Function to show the list of commands
void showCommandList() {
    cout << "Available Commands:" << endl;
    cout << "1. show_the_list_of_commands it prints all commands" << endl;
    cout << "2. import_story {filename.txt} " << endl;
    cout << "   it takes the name of the story and if it exists, it reads that file and adds it to read stories" << endl;
    cout << "3. show_the_list_of_stories " << endl;
    cout << "   it displays the previously read stories sorted by load time" << endl;
    cout << "4. analyze_story {story_index} {output_file_name.txt}" << endl;
    cout << "   it analyzes the specified story and predicts its genre" << endl;
    cout << "5. analyzed_stories_list";
    cout << "   it displays the names of the analyzed stories)" << endl;
    cout << "6. show_story_analysis {story_index}" << endl;
    cout << "   it shows the analysis of the specified story directly from memory" << endl;
    cout << "7. dump_analyzed_stories {output_filename.csv}" << endl;
    cout << "0. exit" << endl;
}

// Function to display the list of stories
void showListOfStories(const vector<pair<string, string>>& stories) {
    if (stories.empty()) {
        cout << "No stories have been imported yet." << endl;
    } else {
        cout << "List of Stories:" << endl;
        for (size_t i = 0; i < stories.size(); ++i) {
            // Capitalize the first letter of the story name
            string storyName = stories[i].second;

            // Remove ".txt" extension from the story name
            size_t extensionPos = storyName.find_last_of(".");
            if (extensionPos != string::npos) {
                storyName = storyName.substr(0, extensionPos);
            }

            if (!storyName.empty()) {
                storyName[0] = toupper(storyName[0]);
                cout << i + 1 << ". " << storyName << endl;
            }
        }
    }
}

// Function to import a story from a file
void importStory(const string& filename, vector<pair<string, string>>& readStories) {
    ifstream storyFile(filename);

    if (!storyFile.is_open()) {
        cerr << "File not found: " << filename << endl;
    } else {
        // Extract the filename from the provided path
        size_t lastSlashPos = filename.find_last_of("/\\");
        string storyName = filename.substr(lastSlashPos + 1);

        // Read the content of the story
        stringstream storyBuffer;
        storyBuffer << storyFile.rdbuf();
        string storyContent = storyBuffer.str();

        // Store both the content and the name of the story in readStories
        readStories.push_back({storyContent, storyName});
        cout << "Story '" << storyName << "' imported successfully." << endl;
        // cout << storyContent;

        storyFile.close();
    }
}
// Function to convert string to lowercase
string toLower(const string& str) {
    string lowerStr = str;
    transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

vector<string> findMostUsedWords(const string& storyContent, const unordered_map<string, vector<pair<string, int>>>& genreKeywordMap, size_t n);
// Function to analyze a story and count occurrences of genre words
void analyzeStory(const string& storyContent, const string& storyName, const unordered_map<string, vector<pair<string, int>>>& genreKeywordMap, const string& outputFilename) {
    // Initialize counts for each genre
    unordered_map<string, int> genreWordCounts;

    // Tokenize the story content into words
    istringstream iss(storyContent);
    string word;

    // Create a set for quick lookup of genre words
    unordered_set<string> genreWords;

    // Populate genreWords set
    for (const auto& genrePair : genreKeywordMap) {
        const vector<pair<string, int>>& keywords = genrePair.second;
        for (const auto& keywordPair : keywords) {
            genreWords.insert(toLower(keywordPair.first));  // Convert keyword to lowercase for case-insensitive comparison
        }

        // Initialize word counts for each genre
        genreWordCounts[genrePair.first] = 0;
    }

    while (iss >> word) {
        // Check if the word is a genre word
        for (const auto& genrePair : genreKeywordMap) {
            const vector<pair<string, int>>& keywords = genrePair.second;

            // Iterate over keywords for the current genre
            for (const auto& keywordPair : keywords) {
                // Extract the keyword from the "keyword,weight" format
                size_t commaPos = keywordPair.first.find(',');
                string keyword = (commaPos != string::npos) ? keywordPair.first.substr(0, commaPos) : keywordPair.first;

                // Check if the current word matches any keyword for the current genre
                if (keyword == word) {
                    // Increment the count for the corresponding genre
                    genreWordCounts[genrePair.first]++;
                    break; // Break after finding the first match to avoid double counting
                }
            }
        }
    }

        // Identify the genre with the highest word count
    string predictedGenre;
    int maxWordCount = 0;

    for (const auto& genreCount : genreWordCounts) {
        const string& genre = genreCount.first;
        int count = genreCount.second;

        if (count > maxWordCount) {
            maxWordCount = count;
            predictedGenre = genre;
        }
    }

    // Print the genre of the story in the terminal
    cout << "The genre of the story " << storyName.substr(0, storyName.find_last_of(".")) << " is " << predictedGenre.substr(0, predictedGenre.find_last_of(".")) << "." << endl;


    // Calculate genre confidence
    int totalWords = accumulate(genreWordCounts.begin(), genreWordCounts.end(), 0,
                                [](int total, const auto& genreCount) { return total + genreCount.second; });
    vector<string> mostUsedWords = findMostUsedWords(storyContent, genreKeywordMap, 5);

    // Open the output file for writing
    ofstream outputFile(outputFilename);
    if (outputFile.is_open()) {
        // Write analysis results to the output file
        outputFile << "Genre, Number of Keywords, Confidence" << endl;

        for (const auto& genreCount : genreWordCounts) {
            const string& genre = genreCount.first;
            int count = genreCount.second;
            double confidence = (totalWords > 0) ? (static_cast<double>(count) / totalWords) * 100.0 : 0.0;

            // Remove ".csv" extension from the genre name
            size_t extensionPos = genre.find_last_of(".");
            string genreName = genre.substr(0, extensionPos);

            outputFile << genreName << ", " << count << ", " << confidence << "%" << endl;
            
            }
            outputFile << "The common keywords are: ";
            for (size_t i = 0; i < mostUsedWords.size(); ++i) {
                outputFile << mostUsedWords[i];

                // Check if it's the last element
                if (i < mostUsedWords.size() - 1) {
                    outputFile << ", ";
                } else {
                    outputFile << ".";
                }
            }
        outputFile.close();
    } 
    
}

// Function to add a story to the analyzed stories list
void addToAnalyzedStoriesList(const string& storyName) {
    analyzedStoriesList.push_back(storyName);
}

// Function to display the list of analyzed stories
void showAnalyzedStoriesList() {
    if (analyzedStoriesList.empty()) {
        cout << "No stories have been analyzed yet." << endl;
    } else {
        cout << "The analyzed stories are: ";

        // Capitalize the first letter of the first story name
        string firstStoryName = analyzedStoriesList[0];
        firstStoryName[0] = toupper(firstStoryName[0]);
        cout << firstStoryName.substr(0, firstStoryName.find_last_of("."));

        // Display the rest of the story names with proper formatting
        for (size_t i = 1; i < analyzedStoriesList.size(); ++i) {
            string storyName = analyzedStoriesList[i];
            // Capitalize the first letter of each story name
            storyName[0] = tolower(storyName[0]);

            // If it's the last story, use "and" instead of a comma
            if (i == analyzedStoriesList.size() - 1) {
                cout << " and " << storyName.substr(0, storyName.find_last_of("."));
            } else {
                cout << ", " << storyName.substr(0, storyName.find_last_of("."));
            }
        }

        cout << endl;
    }
}

// Function to show the analysis of a specific story
void showStoryAnalysis(size_t storyIndex, const vector<pair<string, string>>& readStories, const unordered_map<string, vector<pair<string, int>>>& genreKeywordMap) {
    // Check if the story index is valid
    if (storyIndex >= 1 && storyIndex <= readStories.size()) {
        const string& storyName = readStories[storyIndex - 1].second;

        // Check if the story has been analyzed
        if (find(analyzedStoriesList.begin(), analyzedStoriesList.end(), storyName) == analyzedStoriesList.end()) {
            cerr << "This story has not been analyzed yet. Please use the analyze_story command." << endl;
            return;
        }

        // Subtract 1 to convert from user index (starting from 1) to vector index (starting from 0)
        const string& storyContent = readStories[storyIndex - 1].first;

        // Initialize counts for each genre
        unordered_map<string, int> genreWordCounts;

        // Tokenize the story content into words
        istringstream iss(storyContent);
        string word;

        while (iss >> word) {
            // Check if the word is a genre word
            for (const auto& genrePair : genreKeywordMap) {
                const vector<pair<string, int>>& keywords = genrePair.second;

                // Iterate over keywords for the current genre
                for (const auto& keywordPair : keywords) {
                    // Extract the keyword from the "keyword,weight" format
                    size_t commaPos = keywordPair.first.find(',');
                    string keyword = (commaPos != string::npos) ? keywordPair.first.substr(0, commaPos) : keywordPair.first;

                    // Check if the current word matches any keyword for the current genre
                    if (toLower(keyword) == toLower(word)) {
                        // Increment the count for the corresponding genre
                        genreWordCounts[genrePair.first]++;
                        break; // Break after finding the first match to avoid double counting
                    }
                }
            }
        }

        // Display the analysis information for the specified story
        cout << "Analysis for the story '" << storyName << "':" << endl;
        cout << "Genre, Number of Keywords, Confidence" << endl;

        int totalWords = accumulate(genreWordCounts.begin(), genreWordCounts.end(), 0,
                                    [](int total, const auto& genreCount) { return total + genreCount.second; });

        for (const auto& genreCount : genreWordCounts) {
            const string& genre = genreCount.first;
            int count = genreCount.second;
            double confidence = (totalWords > 0) ? (static_cast<double>(count) / totalWords) * 100.0 : 0.0;

            // Remove ".csv" extension from the genre name
            size_t extensionPos = genre.find_last_of(".");
            string genreName = genre.substr(0, extensionPos);

            cout << genreName << ", " << count << ", " << confidence << "%" << endl;
        }
    } else {
        cerr << "Invalid story index." << endl;
    }
}

// Function to find n most used words in a story based on its genre
vector<string> findMostUsedWords(const string& storyContent, const unordered_map<string, vector<pair<string, int>>>& genreKeywordMap, size_t n) {
    // Tokenize the story content into words
    istringstream iss(storyContent);
    string word;

    // Create a map to store word frequencies
    unordered_map<string, int> wordFrequencies;

    while (iss >> word) {
        // Check if the word is a genre word
        for (const auto& genrePair : genreKeywordMap) {
            const vector<pair<string, int>>& keywords = genrePair.second;

            // Iterate over keywords for the current genre
            for (const auto& keywordPair : keywords) {
                // Extract the keyword from the "keyword,weight" format
                size_t commaPos = keywordPair.first.find(',');
                string keyword = (commaPos != string::npos) ? keywordPair.first.substr(0, commaPos) : keywordPair.first;

                // Check if the current word matches any keyword for the current genre
                if (toLower(keyword) == toLower(word)) {
                    // Increment the count for the corresponding word
                    wordFrequencies[word]++;
                    break; // Break after finding the first match to avoid double counting
                }
            }
        }
    }

    // Sort words by frequency in descending order
    vector<pair<string, int>> sortedWords(wordFrequencies.begin(), wordFrequencies.end());
    sort(sortedWords.begin(), sortedWords.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Get the top n words
    vector<string> mostUsedWords;
    for (size_t i = 0; i < min(n, sortedWords.size()); ++i) {
        mostUsedWords.push_back(sortedWords[i].first);
    }

    return mostUsedWords;
}

// Function to find the content of a story based on its name
const string& findStoryContent(const string& storyName, const vector<pair<string, string>>& readStories) {
    for (const auto& storyPair : readStories) {
        if (storyPair.second == storyName) {
            return storyPair.first;
        }
    }

    throw runtime_error("Story not found.");
}

// Function to dump analyzed stories to a CSV file
void dumpAnalyzedStories(const string& outputFilename, const unordered_map<string, vector<pair<string, int>>>& genreKeywordMap, const vector<pair<string, string>>& readStories) {
    // Open the output file for writing
    ofstream outputFile(outputFilename);

    if (outputFile.is_open()) {
        // Write the header line to the output file
        outputFile << "Story, Genre, Confidence, Romance Words, Mystery Words, Fantasy Words, SciFi Words, Common Keyword 1, Common Keyword 2, Common Keyword 3, Common Keyword 4" << endl;

        // Loop through each analyzed story
        for (const auto& storyPair : readStories) {
            const string& storyName = storyPair.second;
            const string& storyContent = findStoryContent(storyName, readStories);
            
            // Initialize counts for each genre
            unordered_map<string, int> genreWordCounts;

            // Tokenize the story content into words
            istringstream iss(storyContent);
            string word;

            // Populate genreWords set
            unordered_set<string> genreWords;

            // Populate genreWords set
            for (const auto& genrePair : genreKeywordMap) {
                const vector<pair<string, int>>& keywords = genrePair.second;
                for (const auto& keywordPair : keywords) {
                    genreWords.insert(toLower(keywordPair.first));  // Convert keyword to lowercase for case-insensitive comparison
                }

                // Initialize word counts for each genre
                genreWordCounts[genrePair.first] = 0;
            }

            while (iss >> word) {
                // Check if the word is a genre word
                for (const auto& genrePair : genreKeywordMap) {
                    const vector<pair<string, int>>& keywords = genrePair.second;

                    // Iterate over keywords for the current genre
                    for (const auto& keywordPair : keywords) {
                        // Extract the keyword from the "keyword,weight" format
                        size_t commaPos = keywordPair.first.find(',');
                        string keyword = (commaPos != string::npos) ? keywordPair.first.substr(0, commaPos) : keywordPair.first;

                        // Check if the current word matches any keyword for the current genre
                        if (keyword == word) {
                            // Increment the count for the corresponding genre
                            genreWordCounts[genrePair.first]++;
                            break; // Break after finding the first match to avoid double counting
                        }
                    }
                }
            }

            // Identify the genre with the highest word count
            string predictedGenre;
            int maxWordCount = 0;

            for (const auto& genreCount : genreWordCounts) {
                const string& genre = genreCount.first;
                int count = genreCount.second;

                if (count > maxWordCount) {
                    maxWordCount = count;
                    predictedGenre = genre;
                }
            }

            // Calculate genre confidence
            int totalWords = accumulate(genreWordCounts.begin(), genreWordCounts.end(), 0,
                                        [](int total, const auto& genreCount) { return total + genreCount.second; });

            // Write the story info to the output file
            outputFile << storyName.substr(0, storyName.find_last_of(".")) << ", " << predictedGenre.substr(0, predictedGenre.find_last_of(".")) << ", ";
            outputFile << (totalWords > 0 ? (static_cast<double>(maxWordCount) / totalWords) * 100.0 : 0.0) << "%, ";

            // Write counts for each genre
            for (const auto& genreCount : genreWordCounts) {
                outputFile << genreCount.second << ", ";
            }

            // Find and write common keywords
            vector<string> mostUsedWords = findMostUsedWords(storyContent, genreKeywordMap, 4);
            for (size_t i = 0; i < mostUsedWords.size(); ++i) {
                outputFile << mostUsedWords[i];

                // Check if it's the last element
                if (i < mostUsedWords.size() - 1) {
                    outputFile << ", ";
                } else {
                    outputFile << endl; // End the line after the last keyword
                }
            }
        }

        cout << "All analyzed stories dumped in " << outputFilename << "." << endl;
        outputFile.close();
    } else {
        cerr << "Error: Unable to open the output file." << endl;
    }
}


int main() {
    // Step 2: Read Genre-Keyword Mapping from CSV
    unordered_map<string, vector<pair<string, int>>> genreKeywordMap;

    // List of genre CSV files
    vector<string> genreFiles = {"Romance.csv", "Mystery.csv", "Fantasy.csv", "SciFi.csv"};

    // Populate genre-keyword mapping for each CSV file
    for (const string& genreFile : genreFiles) {
        try {
            auto result = readCSV(genreFile);
            genreKeywordMap.insert(result.begin(), result.end());
        } catch (const exception& e) {
            cerr << e.what() << endl;
            return 1;  // Exit with an error code
        }
    }

    // List to store read stories with their names and content
    vector<pair<string, string>> readStories;
    string storyName;

    showCommandList();

    // Main loop for menu
    while (true) {
        // Wait for user input
        cout << "\nEnter a command: ";
        string cmd;
        cin >> cmd;

        // Handle user commands
        if (cmd == "show_the_list_of_commands") {
            showCommandList();
        } else if (cmd == "import_story") {
            string filename;
            cin >> filename;

            importStory(filename, readStories);
        } else if (cmd == "show_the_list_of_stories") {
            // Sort stories by load time before displaying
            sort(readStories.begin(), readStories.end(), [](const auto& a, const auto& b) {
                // Compare using the time of import
                return a.first < b.first;
            });
            showListOfStories(readStories);
        } else if (cmd == "analyze_story") {
            // Get story index and output filename from user input
            size_t storyIndex;
            string outputFilename;
            cin >> storyIndex >> outputFilename;

            // Check if the story index is valid
            if (storyIndex >= 1 && storyIndex <= readStories.size()) {

                // Subtract 1 to convert from user index (starting from 1) to vector index (starting from 0)
                const string& storyContent = readStories[storyIndex - 1].first;
                const string& storyName = readStories[storyIndex - 1].second;
                analyzeStory(storyContent, storyName, genreKeywordMap, outputFilename);
                addToAnalyzedStoriesList(storyName);
            } else {
                cerr << "Invalid story index." << endl;
            }
        } else if (cmd == "analyzed_stories_list") {
            showAnalyzedStoriesList();
        } else if (cmd == "dump_analyzed_stories") {
            string outputFilename;
            cin >> outputFilename;

            dumpAnalyzedStories(outputFilename, genreKeywordMap, readStories);
        } else if (cmd == "exit") {
            cout << "Exiting the program." << endl;
            break;
        } else if (cmd == "show_story_analysis") {
            // Get story index from user input
            size_t storyIndex;
            cin >> storyIndex;
            // Show analysis for the specified story
            showStoryAnalysis(storyIndex, readStories, genreKeywordMap);
        } else {
            cerr << "Error: Unknown command. Type 'show_the_list_of_commands' for options." << endl;
        }
    }


    return 0;
}