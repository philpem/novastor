# DeNova

Tools to decompress ancient NovaStor NovaBack / NovaBackup tape backups

Tested with backups from NovaBack 4, other versions may work.

If this helps you, please let me know: philpem@gmail.com

This project is not authorised, endorsed or related to Novastor or Novaback and has been created with entirely open-source materials. No warranty is given, express or implied.

Based on portions of the IGeoS data analysis tool developed by the Seismology and Geophysics department at the University of Saskatchewan (http://seisweb.usask.ca/SIA/).


## How to use

Compile using `make`.

Use a tool like Tapeimgr (https://github.com/KBNLresearch/tapeimgr) to read the tape.

The first block will be a Novaback tape header, which will contain no data. Subsequent blocks will contain compressed (or not) block headers, which in turn contain file data. Ignore the tape header block and process only the other blocks.

Use `denova` to extract and decompress the file data. Then use `denovafile` to extract the files from the file data. These two commands may be piped into each other:

```
./denova file001.dd | ./denovafile
```

File attributes are not restored, but are printed on the CLI. To log these to a file you may redirect them to a file or pipe them through `tee`.
