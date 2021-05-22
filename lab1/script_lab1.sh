#!/bin/bash
rm -rf 9091
mkdir 9091
cd 9091
mkdir Lopatin
cd Lopatin
date > Dan
date –d ”next Mon” > filedate.txt
cat Dan filedate.txt > result.txt
cat result.txt