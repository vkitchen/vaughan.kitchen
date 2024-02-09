#!/bin/sh

mkdir out
mkdir out/games

cp -r static out/

m4 tmpl/index.m4 > out/index.html
m4 tmpl/cv.m4 > out/resume.html
m4 tmpl/games.m4 > out/games.html
cp tmpl/connect4.html out/games/connect4.html
