# HRPT2APT
HRPT2APT can convert raw16 frames or dundee format HRPT from the [CEDA archive](https://data.ceda.ac.uk/neodc/avhrr_dundee) to analog APT with correct telemetry for use with older software such as WXtoIMG or just if you miss APT.

If you want to encode random images as APT, you may find my [APT-encoder fork](https://github.com/thomasdouwes/apt-encoder) more interesting

## planned features
- MetOp AHRPT input support
- Meteor HRPT input support
- Meteor LRPT input support
- Official MetOp AVHRR FRAC product input support
- PNG, WAV, IQ output options
- File timestamping for WXtoIMG
- Channel switching at terminator
- Other analogue formats? (old meteor APT, SICH APT, ITOS HRPT)

## How to use
HRPT2APT probably only works on linux
### build
```
git clone https://github.com/thomasdouwes/hrpt2apt.git
cd hrpt2apt
mkdir build
cmake ..
make
```
### run
SatDump format (defaults to channels 2/4)
```
./hrpt2apt -i "noaa_hrpt.raw" -r apt.dat
```
or with 1/2 channels selected
```
./hrpt2apt -i "noaa_hrpt.raw" -r apt.dat -a ch1 -b ch2
```
or dundee format
```
./hrpt2apt -i "NOAA-15.dat" -r apt.dat -d
```

### using imagemagick to get PNG
The last line of the command output will print a number of APT lines like
```
1542 APT lines
```

Use the following imagemagick command to generate the image
```
magick -size 2080x<lines> -depth 8 gray:apt.dat apt.png
```
where `<lines>` is the number of APT lines output by hrpt2apt  
On older linux distros you can use the following imagemagick command
```
convert -size 2080x<lines> -depth 8 gray:apt.dat apt.png
```
Replace apt.dat and apt.png with your file names

### GNU radio
To convert to WAV or IQ you need to use the included GNU radio flowcharts  
dat_to_wav.grc for WAV, dat_to_iq.grc for IQ.  
You can use your APT wav in most software, and the IQ file can be played in SDR++ or SatDump