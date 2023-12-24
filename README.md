# Install requirements
1. Install GLUT on Ubuntu
```bash
sudo apt update  
sudo apt-get install freeglut3-dev -y
```
2. Install glfw3
```bash
sudo apt update  
sudo apt-get install libglfw3 -y
sudo apt-get install libglfw3-dev -y
```
3. Install glm
```bash
sudo apt update
sudo apt install libglm-dev -y
```
4. Install glew
```bash
sudo apt update
sudo apt-get install -y libglew-dev
```


# Run the code
```bash
git submodule update --init --recursive
```
```bash
mkdir build
cd build
cmake ..
make
../bin/PIKProject
```