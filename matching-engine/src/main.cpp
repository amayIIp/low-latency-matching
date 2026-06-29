
#include <iostream>

#include <fstream>

#include <sstream>

#include <string>

#include <vector>

#include "lob/order_book.hpp"

std::vector<std::string> split_by_comma(const std::string& input_string) {

    std::vector<std::string> parts;

    std::stringstream string_stream(input_string);

    std::string part;

    while (std::getline(string_stream, part, ',')) {

        parts.push_back(part);
    }

    return parts;
}

void print_order_book_state(const lob::OrderBook& book) {

    std::cout << "\n=== FINAL ORDER BOOK STATE ===\n";
    std::cout << "--- BIDS ---\n";

    for (const auto& [price, orders] : book.bids()) {

        for (const auto& order : orders) {

            std::cout << "Price: " << price << " | Qty: " << order.qty << " (Order ID: " << order.id << ")\n";
        }
    }

    std::cout << "--- ASKS ---\n";

    for (const auto& [price, orders] : book.asks()) {

        for (const auto& order : orders) {

            std::cout << "Price: " << price << " | Qty: " << order.qty << " (Order ID: " << order.id << ")\n";
        }
    }
    std::cout << "==============================\n\n";
}

int main(int argc, char* argv[]) {

    if (argc < 2) {

        std::cerr << "Usage: " << argv[0] << " <commands_file.txt>\n";
        std::cerr << "Format of commands_file.txt lines:\n";
        std::cerr << "  ADD,id,side,price,qty  (e.g., ADD,1,BUY,100,50)\n";
        std::cerr << "  CANCEL,id              (e.g., CANCEL,1)\n";

        return 1;
    }

    std::ifstream file(argv[1]);

    if (!file.is_open()) {

        std::cerr << "Error: Could not open file " << argv[1] << "\n";

        return 1;
    }

    lob::OrderBook book;

    uint64_t sequence_timestamp = 0;

    std::string line;

    while (std::getline(file, line)) {

        if (line.empty()) {
            continue;
        }

        if (line[0] == '#') {
            continue;
        }

        std::vector<std::string> tokens = split_by_comma(line);

        if (tokens.empty()) {
            continue;
        }

        std::string command = tokens[0];

        if (command == "ADD") {

            if (tokens.size() < 5) {
                std::cerr << "Warning: Malformed ADD line skipped: " << line << "\n";
                continue;
            }

            lob::OrderId id = std::stoull(tokens[1]);
            std::string side_str = tokens[2];
            lob::Price price = std::stoll(tokens[3]);
            lob::Qty qty = std::stoll(tokens[4]);

            lob::Side side = lob::Side::Buy;
            if (side_str == "SELL" || side_str == "Sell" || side_str == "S") {
                side = lob::Side::Sell;
            }

            sequence_timestamp++;

            lob::Order order(id, side, price, qty, sequence_timestamp);

            auto trades = book.addOrder(order);

            for (const auto& trade : trades) {
                std::cout << "TRADE,maker_id=" << trade.maker_id
                          << ",taker_id=" << trade.taker_id
                          << ",price=" << trade.price
                          << ",qty=" << trade.qty << "\n";
            }

        }

        else if (command == "CANCEL") {

            if (tokens.size() < 2) {
                std::cerr << "Warning: Malformed CANCEL line skipped: " << line << "\n";
                continue;
            }

            lob::OrderId id = std::stoull(tokens[1]);

            bool success = book.cancelOrder(id);

            if (!success) {
                std::cout << "CANCEL_FAILED,id=" << id << "\n";
            }
        }

        else {
            std::cerr << "Warning: Unknown command skipped: " << command << "\n";
        }
    }

    print_order_book_state(book);

    return 0;
}
