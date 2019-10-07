#!/usr/bin/env perl

use strict;
use warnings;

open my $handle, '<', $ARGV[0];
chomp(my @lines  = <$handle>);
close $handle;

my @expect = ('<!DOCTYPE html>',
'<html>',
'<head>',
'<title>Good Noms</title>',
'</head>',
'<body>',
'<h1>Good Noms</h1>');

my $line_7 = '<h3><a href="../../index.html">Gnoms</a> > <a href="../index.html">Garden</a> > <a href="index.html">Plants</a> >';

if ($#lines < 10) {
	die "File too short";
}

foreach my $line (@lines) {
	#print $line."\n";
}

for (my $i = 0; $i < $#expect; $i++) {
	if ($lines[$i] ne $expect[$i]) {
		print "Got '$lines[$i]'\n";
		print "Expected '$expect[$i]'\n";
		die "Expected lines not present $i";
	}
}

if (index($lines[7], $line_7) == -1) {
	die "Breadcrumb missing";
}

my $plant = substr($lines[7], length $line_7);

if (rindex($plant, '</h3>') == -1) {
	die "Breadcrumb broken";
}

$plant = substr($plant, 0, rindex($plant, '</h3>'));

$plant =~ s/^\s+|\s+$//g;


print "<!DOCTYPE html>\n";
print "<html>\n";
print "<head>\n";
print "<meta charset=\"utf-8\">\n";
print "<link rel=\"stylesheet\" type=\"text/css\" href=\"/main.css\">\n";
print "<title>$plant - Vaughan.Kitchen</title>\n";
print "</head>\n";
print "<body>\n";
print "<h1 class=\"site-logo\"><a href=\"/\">Vaughan Kitchen</a></h1>\n";
print "<h3><a href=\"/\">Home</a> > <a href=\"/garden/\">Garden</a> > <a href=\"/garden/plants/\">Plants</a> > $plant</h3>\n";

for (my $i = 8; $i < $#lines+1; $i++) {
	print "$lines[$i]\n";
}
