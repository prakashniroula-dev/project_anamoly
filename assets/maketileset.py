import os
from PIL import Image

def create_tileset():
    # Configuration
    tile_size = 32
    columns = 9
    rows = 9
    total_tiles = 81
    prefix = "IndustrialTile_"
    output_filename = "IndustrialTileset.png"

    # Calculate final image dimensions
    tileset_width = columns * tile_size
    tileset_height = rows * tile_size

    # Create a new blank transparent image
    tileset = Image.new('RGBA', (tileset_width, tileset_height), (0, 0, 0, 0))

    print(f"Creating a {columns}x{rows} tileset ({tileset_width}x{tileset_height} pixels)...")

    for i in range(1, total_tiles + 1):
        # Format the filename with leading zero (e.g., IndustrialTile_01.png)
        filename = f"{prefix}{i:02d}.png"
        
        if not os.path.exists(filename):
            print(f"Warning: {filename} not found! Skipping.")
            continue

        try:
            # Open the tile image
            with Image.open(filename) as tile:
                # Ensure the tile is 32x32
                if tile.size != (tile_size, tile_size):
                    print(f"Warning: {filename} is {tile.size}, expected ({tile_size}, {tile_size}).")
                
                # Calculate grid position (0-indexed)
                index = i - 1
                col = index % columns
                row = index // columns
                
                # Calculate pixel coordinates
                x = col * tile_size
                y = row * tile_size
                
                # Paste the tile into the tileset canvas
                tileset.paste(tile, (x, y))
                print(f"Pasted {filename} at ({x}, {y})")
                
        except Exception as e:
            print(f"Error processing {filename}: {e}")

    # Save the final image
    tileset.save(output_filename)
    print(f"\nSuccess! Tileset saved as '{output_filename}'")

if __name__ == "__main__":
    create_tileset()
