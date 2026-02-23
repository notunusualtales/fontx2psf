# fontx2psf

fontx2psf converts hankaku fonts in FONTX format to PSF format.

## Build
```
$ make
```

## Usage
```
Usage: ./fontx2psf [-p psfver] [-m margin] [-i height] [-N]
    -p psfver: 1 or 2 (default: 2)
    -m margin: 0 - 8 (default: 0)
    -i height: the font height (16,19,24) for PC DOS fonts
    -N       : no magic in the header
```

### Convert to PSF2 format
```
$ ./fontx2psf < fontx.fnt > psfont.psf
```

### Convert to PSF1 format
```
$ ./fontx2psf -p 1 < fontx.fnt > psfont.psf
```

### Convert to PSF2 format with margin 4
```
$ ./fontx2psf -m 4 < fontx.fnt > psfont.psf
```
