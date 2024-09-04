import os, sys, time
from tkinter import Tk, Label
from PIL import Image, ImageTk

# Create a Tkinter window
root = Tk()

# Label to display the image
label = Label(root)
label.pack()

def update_image(image_path):
    """Update the image displayed in the Tkinter window."""
    image = Image.open(image_path)
    image_tk = ImageTk.PhotoImage(image)
    label.config(image=image_tk)
    label.image = image_tk

def get_image_files(directory):
    """Return a sorted list of image files in the directory."""
    return sorted([f for f in os.listdir(directory) if f.endswith(('png', 'jpg', 'jpeg', 'gif'))])

def main(imageDir, delay):
    last_displayed = None
    while True:
        image_files = get_image_files(imageDir)
        if image_files:
            latest_image = os.path.join(imageDir, image_files[-1])
            if latest_image != last_displayed:
                update_image(latest_image)
                last_displayed = latest_image
        root.update_idletasks()
        root.update()
        time.sleep(delay)

if __name__ == "__main__":
    if len(sys.argv) != 8 :
        print('Usage: python3 %s <dir> <delay> w h x y title' % sys.argv[0])
        sys.exit(0)

    imgDir = sys.argv[1]
    delay = float(sys.argv[2])
        
    window_width = int(sys.argv[3])
    window_height = int(sys.argv[4])
    position_x = int(sys.argv[5])
    position_y = int(sys.argv[6])
    title = sys.argv[7]
    root.geometry(f"{window_width}x{window_height}+{position_x}+{position_y}")
    root.title(title)
        
    main(imgDir, delay)
