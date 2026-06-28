# Import the time library to introduce delays between screen refreshes
import time
# Import the random library to generate fluctuating price levels and order quantities
import random
# Import the os library to clear the terminal screen on each refresh loop
import os

# Define a function to print a beautiful, real-time ASCII representation of an order book
def print_live_book():
    # Set the starting reference price for our mock book
    ref_price = 10000
    
    # Loop indefinitely to simulate a live market feed
    while True:
        # Clear the terminal screen using standard shell command ('clear' for Linux/macOS, 'cls' for Windows)
        os.system('clear' if os.name != 'nt' else 'cls')
        
        # Fluctuate the reference price slightly using random steps to simulate price movements
        ref_price += random.randint(-2, 2)
        
        # Print a styled title header
        print("=" * 50)
        print(f"   LOB MATCHING ENGINE - LIVE ORDER BOOK DEPTH")
        print(f"   Reference Price: {ref_price} Ticks")
        print("=" * 50)
        
        # Generate mock asks (Sells) above the reference price, sorted from highest to lowest price
        # Sellers want high prices, so the highest sell rests at the top of our display.
        print("\n--- ASKS (Sells) ---")
        for offset in sorted([random.randint(1, 10) for _ in range(5)], reverse=True):
            # Calculate the ask price tick
            ask_price = ref_price + offset
            # Generate a random quantity for this level
            qty = random.randint(10, 150)
            # Create a visual bar representation of the quantity using ASCII blocks
            bar = "█" * (qty // 10)
            # Print the formatted price, quantity, and the visual bar in red (using ANSI color code \033[91m)
            print(f"Price: {ask_price} | Qty: {qty:<5} | \033[91m{bar:<15}\033[0m")
            
        # Print a divider to show the spread (the gap between highest buy and lowest sell)
        print("-" * 50)
        
        # Generate mock bids (Buys) below the reference price, sorted from highest to lowest
        # Buyers want high prices, so the best buy (highest bid) sits right below the spread.
        print("--- BIDS (Buys) ---")
        for offset in sorted([random.randint(1, 10) for _ in range(5)], reverse=True):
            # Calculate the bid price tick
            bid_price = ref_price - offset
            # Generate a random quantity for this level
            qty = random.randint(10, 150)
            # Create a visual bar representation of the quantity
            bar = "█" * (qty // 10)
            # Print the formatted price, quantity, and the visual bar in green (using ANSI color code \033[92m)
            print(f"Price: {bid_price} | Qty: {qty:<5} | \033[92m{bar:<15}\033[0m")
            
        print("=" * 50)
        print("Press Ctrl+C to exit the live visualization demo.")
        
        # Wait/sleep for 0.5 seconds before updating the display with new data
        time.sleep(0.5)

# Entry point of the script execution
if __name__ == "__main__":
    # Wrap the loop in a try-except block to handle keyboard interrupt exits gracefully
    try:
        # Call the visualization function
        print_live_book()
    # Capture keyboard interrupt (Ctrl+C)
    except KeyboardInterrupt:
        # Print exit log
        print("\nExiting visualization demo. Thank you!")
