# 8/2/2025 UPDATE
I Have made a full code base refactor on this repo. This includes the use of newer cpp functionality,
.bmp stenography updates, ive added .wav stenography, and command line parsing. Ive also reworked the 
Console header to be a lot easier, better and use the newer print functions.

### Command Line Update
- Ive added command line functionality, the arguments are as followed
- [i/e/h] -filepath -filepath <- general format
- [i] -bytecode input file -implant file <- Implant data
- [e] -implanted file -bytecode output file <- Extract data
- [h] <- show commands

# Performace Update
Using std::span to wrap the file data, this removes unwanted copying and moving of the underlying vector used to 
hold the file data. I also reworked Main to be better and more responsive.

# BMP Updates
Ive removed the limitation of 24bit bmp images, and this now uses the 2 LSB's(Least Significant Bits) of each byte to
hide the data being implanted. This has removed artifacting to the naked eye.

# WAV Update
Ive added the option to hide data in .wav audio files, this also uses the 2 LSB'sof each byte to hide the data. I didnt
notice any crazy audio changes at all if any. Currently this only works on PCM format but i have the extensible struct
included if people want to make changes to accept other formats.

the formatting of the byte code used for implanted HASN'T changed it must be as followed in the image below. I.E '0xFF' or 'FF'.
![Screenshot 2024-05-12 092208](https://github.com/Eremetic/Bmp-Stenography/assets/146580877/716c81f4-70af-42ab-908d-8e96238498af)



