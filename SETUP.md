# Development setup guide.

If you're developing using YouCompleteMe on Vim, the following instructions will
help get you started.

```bash
git clone git@github.com:DanielIndictor/yachip8.git
cd yachip8
mkdir build
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j`nproc`
cd ..
ln -s build/compile_commands.json
echo "build/" >> .git/info/exclude
git submodule init
git submodule update
```

You'll also want to install SFML to run the emulator. On Debian
and other systems, this is usually just an apt-get away:
```
apt update
apt install libsfml-dev
```
