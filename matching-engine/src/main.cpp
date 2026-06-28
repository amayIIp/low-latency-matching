// Include the input/output stream library to read from standard input and write to standard output
#include <iostream>
// Include the file stream library to open and read command files
#include <fstream>
// Include stringstream to parse comma-separated fields easily
#include <sstream>
// Include string to use std::string class for string variables
#include <string>
// Include vector to manage lists of strings or trades
#include <vector>
// Include order book and order definitions to interact with the matching engine logic
#include "lob/order_book.hpp"

// Helper function to split a string by a character delimiter (like a comma)
// This is a common requirement in C++ since there is no built-in split() method like in Python.
std::vector<std::string> split_by_comma(const std::string& input_string) {
    // A vector to hold our split string parts
    std::vector<std::string> parts;
    // A stream wrapper around the string to read from it sequentially
    std::stringstream string_stream(input_string);
    // A temporary string variable to hold each part as we extract it
    std::string part;
    // Read from the string stream until the next comma delimiter, saving the token to part
    while (std::getline(string_stream, part, ',')) {
        // Add the extracted token to our parts list
        parts.push_back(part);
    }
    // Return the list of tokens
    return parts;
}

// Function to print the current state of the order book (bids and asks lists)
void print_order_book_state(const lob::OrderBook& book) {
    // Print the bids section header
    std::cout << "\n=== FINAL ORDER BOOK STATE ===\n";
    std::cout << "--- BIDS ---\n";
    // Loop through each bid price level from highest to lowest
    for (const auto& [price, orders] : book.bids()) {
        // Loop through each resting order at this price level
        for (const auto& order : orders) {
            // Print the price, quantity, and order ID of each resting buy order
            std::cout << "Price: " << price << " | Qty: " << order.qty << " (Order ID: " << order.id << ")\n";
        }
    }
    // Print the asks section header
    std::cout << "--- ASKS ---\n";
    // Loop through each ask price level from lowest to highest
    for (const auto& [price, orders] : book.asks()) {
        // Loop through each resting order at this price level
        for (const auto& order : orders) {
            // Print the price, quantity, and order ID of each resting sell order
            std::cout << "Price: " << price << " | Qty: " << order.qty << " (Order ID: " << order.id << ")\n";
        }
    }
    std::cout << "==============================\n\n";
}

// The main entry point of the application
int main(int argc, char* argv[]) {
    // Check if the user failed to supply a command file argument
    if (argc < 2) {
        // Print usage instructions explaining how to run the CLI tool
        std::cerr << "Usage: " << argv[0] << " <commands_file.txt>\n";
        std::cerr << "Format of commands_file.txt lines:\n";
        std::cerr << "  ADD,id,side,price,qty  (e.g., ADD,1,BUY,100,50)\n";
        std::cerr << "  CANCEL,id              (e.g., CANCEL,1)\n";
        // Return a non-zero exit code to signal error status
        return 1;
    }

    // Open the commands file using the filename passed in the command line argument
    std::ifstream file(argv[1]);
    // If the file could not be opened (e.g., file not found or permission issues)
    if (!file.is_open()) {
        // Print an error message with the filename
        std::cerr << "Error: Could not open file " << argv[1] << "\n";
        // Return a non-zero exit code
        return 1;
    }

    // Create an instance of our OrderBook matching engine
    lob::OrderBook book;
    // An incrementing counter to assign a unique time priority timestamp to each new order
    uint64_t sequence_timestamp = 0;

    // A string to hold each line read from the file
    std::string line;
    // Read the file line-by-line until the end of the file is reached
    while (std::getline(file, line)) {
        // If the line is empty (e.g. blank lines), skip it to avoid parser errors
        if (line.empty()) {
            continue;
        }

        // If the first character is a hash symbol, it is a comment, so skip the line
        if (line[0] == '#') {
            continue;
        }

        // Split the line by comma into tokens
        std::vector<std::string> tokens = split_by_comma(line);
        // Check if there are no tokens in the line
        if (tokens.empty()) {
            continue;
        }

        // Extract the command name (ADD or CANCEL) from the first token
        std::string command = tokens[0];

        // If the command is ADD...
        if (command == "ADD") {
            // Verify we have all 5 required fields: ADD, id, side, price, qty
            if (tokens.size() < 5) {
                std::cerr << "Warning: Malformed ADD line skipped: " << line << "\n";
                continue;
            }

            // Convert string tokens to their respective types (using std::stoull and std::stoll)
            lob::OrderId id = std::stoull(tokens[1]);
            std::string side_str = tokens[2];
            lob::Price price = std::stoll(tokens[3]);
            lob::Qty qty = std::stoll(tokens[4]);

            // Determine the enum Side value based on the side string
            lob::Side side = lob::Side::Buy;
            if (side_str == "SELL" || side_str == "Sell" || side_str == "S") {
                side = lob::Side::Sell;
            }

            // Increment our sequence timestamp for time priority ordering
            sequence_timestamp++;

            // Construct the Order object using the parsed fields and sequence timestamp
            lob::Order order(id, side, price, qty, sequence_timestamp);

            // Feed the order into our matching engine and collect any executed trades
            auto trades = book.addOrder(order);

            // Print each executed trade to the standard output
            for (const auto& trade : trades) {
                std::cout << "TRADE,maker_id=" << trade.maker_id 
                          << ",taker_id=" << trade.taker_id 
                          << ",price=" << trade.price 
                          << ",qty=" << trade.qty << "\n";
            }

        } 
        // If the command is CANCEL...
        else if (command == "CANCEL") {
            // Verify we have both required fields: CANCEL, id
            if (tokens.size() < 2) {
                std::cerr << "Warning: Malformed CANCEL line skipped: " << line << "\n";
                continue;
            }

            // Convert the ID token to an unsigned 64-bit integer
            lob::OrderId id = std::stoull(tokens[1]);

            // Attempt to cancel the order in the order book
            bool success = book.cancelOrder(id);
            // If the cancellation failed, log a warning (order might already be fully matched or didn't exist)
            if (!success) {
                std::cout << "CANCEL_FAILED,id=" << id << "\n";
            }
        } 
        // If the command is unknown, log a warning
        else {
            std::cerr << "Warning: Unknown command skipped: " << command << "\n";
        }
    }

    // Print the final state of the order book after all commands have been processed
    print_order_book_state(book);

    // Return 0 to indicate successful execution
    return 0;
}
