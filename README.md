flt2mp4
This c program will generate an *.mp4 file from a set of *.flt files that capture flood depth 
at sequential time steps, along with an *.flt file for the topographic heights.

#Basic information

This program utilizes the package ffmpeg and libpng, which needs to be installed for compilation to be succesful.

#Linux Compilation

gcc -o flt2mp4 flt2mp4.c -lpng -lm

#MacOS Compilation after homebrew installation of libpng

gcc -I/opt/homebrew/include -L/opt/homebrew/lib -lpng -o flt2mp4 flt2mp4.c

#Usage

This assumes the directory structure of PRIMo, whereby project data are stored with project names
such as "miami_gate", as shown below:

primo_gpu/

---------miami_gate/

-------------------raster/

-------------------vector/

---------src/

---------bin/

---------flt2mp4.c

---------flt2mp4

Note there that the executable, "flt2mp4", sits within the "primo_gpu" directory


Usage proceeds as follows:

./flt2mp4 projectname

./flt2mp4 miami_gate

#Output

This will generate flood depth .png files and an *.mp4 file that is saved within the project directory.

#Adjusting graphic coloring

The pink coloring is scaled using the variable "pink_depth". Adjust as needed to optimize graphics.
The grayscale coloring can be adjusted using "zmin" and "zmax". Adjust as needed to optimize graphics.
Recompile code after adjusting parameter values.


