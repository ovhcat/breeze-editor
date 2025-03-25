<div align="center">
  <h1>Text Editor</h1>
  <h4><i>Simple Lightweight Text Editor in C</i></h4>
</div>




```sh
# Update the apt list
sudo apt update

# Update the system to be sure
sudo apt upgrade

## reboot if needed
## sudo reboot now


# install req. packages
#  libgtk-3-dev
#  libgtksourceview-3.0-dev
#  build-essential
sudo apt install libgtk-3-dev libgtksourceview-3.0-dev build-essential


# Todo: check this
# original gcc -o breeze-editor breeze-editor.c `pkg-config --cflags --libs gtk+-3.0 gtksourceview-3.0`
# changeed gcc breeze-editor.c -o breeze-editor $(pkg-config --cflags --libs gtk+-3.0 gtksourceview-3.0)

gcc breeze-editor.c -o breeze-editor $(pkg-config --cflags --libs gtk+-3.0 gtksourceview-3.0)

# To run it:
./breeze-editor
```
