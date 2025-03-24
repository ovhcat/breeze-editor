sudo apt update
sudo apt install libgtk-3-dev libgtksourceview-3.0-dev build-essential


gcc -o breeze-editor breeze-editor.c `pkg-config --cflags --libs gtk+-3.0 gtksourceview-3.0`


./breeze-editor


