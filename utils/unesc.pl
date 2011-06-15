#!/usr/bin/perl
#*=-------------------------------------------------------------------
# Andrew Collington                            Created: 25th May, 1999
# amnuts@talker.com                            Revised: 25th May, 1999
#*=-------------------------------------------------------------------
# unesc.pl : Remove all ^M control codes from all the files in a given
# ver 1.0  : directory without prompting.
#*=-------------------------------------------------------------------

$indir=".";

opendir(DIR,$indir);
local(@filenames)=readdir(DIR);
closedir(DIR);

foreach $file (@filenames) {
  if (-d $file) { next; }
  open(F,"$file");
  @lines=<F>;
  close(F);
  print "processing $file\n";
  open(FP,">${file}X");  
  foreach $line (@lines) {
    $line=~s/\cM\n/\n/g;
    print FP $line;
    }
  close(FP);
  rename("${file}X",$file);
  }

