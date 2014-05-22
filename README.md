This cinder block is a static lib version of OpenAL-soft. 
It also contains helper functions to initialize OpenAL, load and play a .wav file.
The block supports Windows (vs2012) and Mac. Currently only 32 bit supported.

Note: be sure to define AL_LIBTYPE_STATIC in your project when using this library.


OpenAL Soft 1.15.1

Link to OpenAL Soft:
http://kcat.strangesoft.net/openal.html

OpenAL Soft is licensed under GPLv2. Because this block has chosen to use it as a static library, any code that links with it is required by the license to distribute source code to allow compilation with a different version of the library. This block can be easily modified to use the dynamic version of the library which would allow linking without requiring source distribution of the including project.