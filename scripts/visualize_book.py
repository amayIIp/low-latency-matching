
import time

import random

import os

def print_live_book():

    ref_price = 10000

    while True:

        os.system('clear' if os.name != 'nt' else 'cls')

        ref_price += random.randint(-2, 2)

        print("=" * 50)
        print(f"   LOB MATCHING ENGINE - LIVE ORDER BOOK DEPTH")
        print(f"   Reference Price: {ref_price} Ticks")
        print("=" * 50)

        print("\n--- ASKS (Sells) ---")
        for offset in sorted([random.randint(1, 10) for _ in range(5)], reverse=True):

            ask_price = ref_price + offset

            qty = random.randint(10, 150)

            bar = "█" * (qty // 10)

            print(f"Price: {ask_price} | Qty: {qty:<5} | \033[91m{bar:<15}\033[0m")

        print("-" * 50)

        print("--- BIDS (Buys) ---")
        for offset in sorted([random.randint(1, 10) for _ in range(5)], reverse=True):

            bid_price = ref_price - offset

            qty = random.randint(10, 150)

            bar = "█" * (qty // 10)

            print(f"Price: {bid_price} | Qty: {qty:<5} | \033[92m{bar:<15}\033[0m")

        print("=" * 50)
        print("Press Ctrl+C to exit the live visualization demo.")

        time.sleep(0.5)

if __name__ == "__main__":

    try:

        print_live_book()

    except KeyboardInterrupt:

        print("\nExiting visualization demo. Thank you!")
