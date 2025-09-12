from glob import glob
import numpy as np
from PIL import Image
import os


# Obtain all RGB565 raw image buffers transferred by Arduino Nicla Vision as text (.txt) files.
path = "../apriltag_samples"
raw_buffers = glob(path + "/*.txt")

# Convert each retrieved RGB565 raw image buffer to a JPG image and save the generated image files to the jpg_converted folder.
for buf in raw_buffers:
    # Define the required image information.
    loc = path + "/jpg_converted/" + buf.split("/")[-1].split(".")[0] + ".jpg"
    size = (320,320)
    # Conversion: RGB565 (uint16_t) to RGB (3x8-bit pixels, true color)
    raw = np.fromfile(buf).byteswap(True)
    file = Image.frombytes('RGB', size, raw, 'raw', 'BGR;16', 0, 1)
    # Save the generated JPG image file.
    file.save(loc)
    print("Converted: " + loc)
    # After converting and saving the JPG image file successfully, remove the converted RGB565 raw image buffer (.txt).
    os.remove(buf);
