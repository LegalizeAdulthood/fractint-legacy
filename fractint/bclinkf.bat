attrib -r fractint.exe
TLINK /L..\lib/o @bcfract.lnk > bcfract.log
attrib +r fractint.exe
type bcfract.log | more

