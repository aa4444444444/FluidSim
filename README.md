# FluidSim

How to run:
1) Download the files and extract them (or use a git clone)
2) Open the .sln file in Visual Studio
3) Acquire the other necessary files and point to them correctly
4) Run the solution

The other files you'll need are:
1) GLEW (https://glew.sourceforge.net/)
2) GLFW (https://www.glfw.org/)
3) Eigen (https://eigen.tuxfamily.org/index.php?title=Main_Page)
4) GLAD (https://glad.dav1d.de/)

Once you have these files you will likely need to edit the properties of the solution to point to them correctly. You will likely need to edit the **Include Directories** and **Library Directories** under Configuration Properties->VC++ Directories. You will also probably have to edit the **Additional Dependencies** under Configuration Properties->Linker->Input (I recommend adding the glew static library, glew32s.lib, and then adding a GLEW_STATIC preprocessor directive). You will also have to download the Eigen files and change the include statments to point to your location of **Eigen/Dense**. 
