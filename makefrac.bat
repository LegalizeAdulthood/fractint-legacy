rem the SET commands below are here to forcibly turn off CodeView
rem information (which some of my other MAKEs forcibly turn on).
set CL=/AM /FPi /Oait
set MASM=/MX
set LINK=/ST:4096
make fractint.mak
