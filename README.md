# DiskCopier
 A windows commandline tool that copies CDs/DVD/BDs bit for bit.
 
 Other tools extract all the files from the disk then creates an iso file from the extracted files. This means that the order of files on disk will be different. This tool does not extract any files, it simply copies every single bit on the disk directly to a file. This can also circumvent some old DRM. The tool also keeps the disk stucture even if it has bad sectors. It will just write zeros there.
 
 If the tool encounters a bad sector it will write zeros for that sector and mark it as a bad sector.
 When the tool is finished with reading all the sectors it will go into a loop trying to read the bad sectors again and again, until there is no bad sectors left. The user can cancel at anytime with CTRL+C, this should also be used if the sector is unreadable, because the program will try to read it indefinetly.


Syntax:
```
DiskCopier <outputfile> <drive>
```

The drive has to be formatted like so: `\\.\X:` where `X` is your drive letter.
