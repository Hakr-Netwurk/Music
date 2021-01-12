# Music
A lightweight CLI music playing program that has better shuffling than standard music players. Isn't complete yet, many things will break. With that said, "normal" mp3 and wav files can be played.

# Using
To use a non-released version of Music (they most likely will at least work), first get the `Music.exe` file from `Music\x64\Release`, then the `dlls.zip` file. Unzip `dlls.zip` to the same directory as `Music.exe`, and run `Music.exe`.  
To use a released version of Music, follow the same procedure as above, except get the files from the release page.  

**PLEASE NOTE: ** Due to a bug with which we are currently investigating, the precompiled binaries may not work for some users (they might just load, hang a few seconds, and then crash). If this is the case for you, your best bet would be to use Visual Studio 2019 to rebuild the source. When building, set the two dialogs to `x64` and `Release`. Then press `F7` to build. Then, you can go back into `x64\Release`, from where you should now be able to run `Music.exe`. (Please remember `dlls.zip`!)
  
Installer might be a thing in the future, unsure for now.  
  
Written by: [supsm](http://github.com/supsm)  
Helper(s): [Vbbab](http://github.com/Vbbab)  
