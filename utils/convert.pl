#!/usr/bin/perl

#=-------------------------------------------------------------------
#= Filename: convert.pl
#= Author  : Andrew Collington, amnuts@talker.com
#=
#= Created : 16th November, 1997
#= Revised : 16th November, 1997
#= Version : 1.0
#=
#= This program allows you to convert your user files from NUTS 3.3.3
#= to Amnuts 1.4.1.  This will only convert standard NUTS files - ie,
#= ones that you have not added any extra strings to.
#=-------------------------------------------------------------------
#= This is original code, but feel free to distribute it to whoever
#= wants it.  Just please keep this text at the top of the program to
#= show that it is my original program.  Thanks :)
#=-------------------------------------------------------------------
#= Tested only with perl version 5.003 so I apologize if it doesn't
#= work on any other versions.
#=-------------------------------------------------------------------


$userdir='../userfiles';  # where they are located relative to prog.


opendir(DIR,$userdir) || die "Can't open $userdir: $!\n";
local(@filenames) = readdir(DIR);
closedir(DIR);

@filenames=sort(@filenames);

# Get list of all .D user files
foreach $file (@filenames) {
    if ($file=~/\.D/) {
	$file=~s/\.D//;
	push(@users,$file);
    }
}
undef(@filenames);


# Alter the actual userfiles
$count=0;
foreach $name (@users) {
    chomp($name);
    open(FPI,"$userdir/$name.D") || die "Could not load userfile $name: $!\n";
    @lines=<FPI>;
    close(FPI);
    open(FPO,">tempuser") || die "Could not open temphelp: $!\n";
    print FPO $lines[0];
    chomp($lines[1]);
    @words=split(/ /,$lines[1]);
    print FPO $lines[1] . " 0 0 $words[4] 0 0 0\n";
    $exp=time();
    print FPO "0 0 0 23 1 1 0 0 0 0\n0 0 0 0 0 0 0 0\n0 0 0 0 6 10\n1 $exp\n";
    chomp($lines[2]);
    print FPO $lines[2] . " noroom #NONE\n";
    print FPO $lines[3];
    print FPO $lines[4];
    print FPO $lines[5];
    print FPO "#UNSET\n#UNSET\n";
    close(FPO);
    rename("tempuser","$userdir/$name.D");
    print "Converted = $name.D\n";
    ++$count;
}

print "Altered -=[ $count ]=- user files.\n";
