#!/usr/bin/perl

#=-------------------------------------------------------------------
#= Filename: alteru.pl
#= Author  : Andrew Collington, amnuts@talker.com
#=
#= Created : 6th November, 1997
#= Revised : 16th November, 1997
#= Version : 1.3
#=
#= This program allows you to added strings, variables and whatever
#= to a user structure.  The program doesn't require you to be using
#= any particular talker code, as it'll work for any userfile.
#=-------------------------------------------------------------------
#= This is original code, but feel free to distribute it to whoever
#= wants it.  Just please keep this text at the top of the program to
#= show that it is my original program, and also my text at the top
#= of the program when it runs.  Thanks :)
#=-------------------------------------------------------------------
#= Tested only with perl version 5.003 so I apologize if it doesn't
#= work on any other versions.
#=-------------------------------------------------------------------


#=-- Set up some variables ------------------------------------------

$userdir='../userfiles';  # what directory to load files from
$backup='USERS';       # what name you want the backup file to have
$filetype='D';         # what name of files you want to alter
$default_content='#UNSET';  # What to add to a line if nothing was given
$dateprogram='/bin/date';
&getdate;

#=-------------------------------------------------------------------



print "+-------------------------------------------------------------------+\n";
print "| This is a Perl program to alter all the userfiles for your talker |\n";
print "+-------------------------------------------------------------------+\n";
print "| Andrew Collington            |    Amnuts : amnuts.talker.com 2402 |\n";
print "| email: amnuts\@talker.com     |          http://amnuts.talker.com/ |\n";
print "+-------------------------------------------------------------------+\n\n";

opendir(DIR,$userdir) || die "ERROR: Cannot open the directory '$userdir': $!\n";
local(@filenames)=readdir(DIR);
closedir(DIR);
@filenames=sort(@filenames);

# Get list of all $filetype user files
foreach $file (@filenames) {
    if ($file=~/\.$filetype/) {
	$file=~s/\.$filetype//;
	push(@users,$file);
    }
}
# free up some memory now we don't need @filenames
undef(@filenames);

$tmp=@users;
print "+-------------------------------------------------------------------+\n";
printf "| Loading file from directory: %-20s File type: .%-3s |\n",$userdir,$filetype;
printf "| Number of files to alter   : %-4d                                 |\n",$tmp;
print "+-------------------------------------------------------------------+\n";

#=-- Give and example of userfile before change ---------------------

open(FP,"$userdir/$users[1].$filetype") || die "Could not open userfile '$userdir/$users[1].$filetype' for example: $!\n";
print "\nAn example userfile looks like:\n\n";
$line_cnt=0;
while(<FP>) {
    chomp($_);
    printf "   %2s> $_\n",++$line_cnt;
} # End while
close(FP);

#=-- Ask user what line to change and what to add -------------------

$is_user_ok="no";
while($is_user_ok=~/^n/i) {
    $is_answer_ok="no";
    while($is_answer_ok=~/^n/i) {
	print "\nLine to add to [1-$line_cnt, 'a' to append] : ";
	$add_to_line=<STDIN>;
	chomp($add_to_line);
	if (($add_to_line=~/^[b-zB-Z]/) || ($add_to_line>$line_cnt) || ($add_to_line eq "0") || ($add_to_line eq "")) {
	    $is_answer_ok="no";
	    print "There was an error with the line count.  Try again.\n";
	}
	else { $is_answer_ok="yes"; }
    } # End while
    print "Add the following : ";
    $add_content=<STDIN>;
    chomp($add_content);
    if ($add_content eq "") {
	print "Nothing was entered for the content.  Adding the default '$default_content'.\n";
	$add_content=$default_content;
    }

    #=-- Give example of userfile after change --------------------------

    open(FP,"$userdir/$users[1].$filetype") || die "Could not open userfile '$userdir/$users[1].$filetype' for example: $!\n";
    print "\nAn example of the new userfiles looks like:\n\n";
    $line_cnt=0;
    while(<FP>) {
	++$line_cnt;
	chomp($_);
	if ($line_cnt==$add_to_line) { printf "   %2s> $_ $add_content\n",$line_cnt; }
	else { printf "   %2s> $_\n",$line_cnt; }
    } # End while
    close(FP);
    if ($add_to_line=~/^a/i) { printf "   %2s> $add_content\n",++$line_cnt; }
    printf "\nIs this all ok? [y/n/q] : ";
    $is_user_ok=<STDIN>;
    chomp($is_user_ok);
    if ($is_user_ok=~/^q/i) {
	print "\nYou have quit the program.  No userfiles were altered.\n\n";
	print "+-------------------------------------------------------------------+\n";
	print "|               Thank you for using this program                    |\n";
	print "|     If you have any questions or comments, please email me        |\n";
	print "+-------------------------------------------------------------------+\n\n";
	exit(1);
    }
    if ($is_user_ok=~/^y/i) { $is_user_ok="yes"; }
    else { $is_user_ok="no"; }
}  # End while

#=-- Ask to backup the files ----------------------------------------

$is_answer_ok="error";
while(!($is_answer_ok=~/^[nNyY]/)) {
    print "Do you wish to back up the files first? [y/n] : ";
    $backup_files=<STDIN>;
    chomp($backup_files);
    if ($backup_files=~/^y/i) {
	system("tar -cf $backup$date.tar $userdir/*.$filetype");
	system("gzip $backup$date.tar");
	$is_answer_ok="yes";
    }
    elsif ($backup_files=~/^n/i) { $is_answer_ok="no"; }
    else {
	print "You must select either 'y' or 'n'.\n";
	$is_answer_ok="error";
    }
}
if ($backup_files=~/^y/i) {
    print "You backed up the old files with the name: $backup$date.tar.gz\n\n";
}
else {
    print "You did not back up the old files.\n\n";
}

#=-- Change the files and output results ----------------------------

($ucnt,$ecnt)=&change_files;
print "\n+-------------------------------------------------------------------+\n";
printf "|      Change Files : %-4d                  Error Count : %-4d      |\n",$ucnt,$ecnt;
print "+-------------------------------------------------------------------+\n\n";
print "+-------------------------------------------------------------------+\n";
print "|               Thank you for using this program                    |\n";
print "|     If you have any questions or comments, please email me        |\n";
print "+-------------------------------------------------------------------+\n\n";


#=-- End of the program ---------------------------------------------





#=-------------------------------------------------------------------
#= sub : change_files
#=     : This will change all the userfiles.  Depends upon the vars
#=     : $add_to_line and $add_content.
#=     : Returns two numbers - $change_count and $error_count
#=-------------------------------------------------------------------

sub change_files {
    my($change_count,$error_count);
    $change_count=0;  $error_count=0;
    if ($add_to_line=~/^a/i) {
	foreach $name (@users) {
	    chomp($name);
	    unless (open(FPI,">>$userdir/$name.$filetype")) {
		print "Could not load file '$userdir/$name.$filetype' to append to: $!\n";
		$error_count++;
		next;
	    }
	    print FPI "$add_content\n";
	    close(FPI);
	    $change_count++;
        }
    } # end if
    else {
	foreach $name (@users) {
	    chomp($name);
	    unless (open(FPI,"$userdir/$name.$filetype")) {
	    print "Could not load file '$userdir/$name.$filetype': $!\n";
	    $error_count++;
	    next;
	}
	    unless (open(FPO,">tempuser")) {
		print "Could not open file 'tempuser' for writing to: $!\n";
		$error_count++;
		close(FPI);
		next;
	    }
	    $i=1;
	    while (<FPI>) {
		if ($i==$add_to_line) {
		    chomp($_);
		    print FPO $_ . " " . $add_content . "\n";
	        }
		else { print FPO $_; }
		++$i;
	    }
	    close(FPI);  close(FPO);
	    ++$change_count;
	    rename("tempuser","$userdir/$name.D");
        }
    } # end else
    return($change_count,$error_count);
}



#=-------------------------------------------------------------------
#= sub : getdate
#=     : requires nothing, returns date to append to backup filename
#=     : of the userfiles
#=-------------------------------------------------------------------

sub getdate {
    my($day);
    $day=`$dateprogram +"%d"`;
    chomp($day);
    unless ($day == 10 || $day == 20 || $day == 30) {
        $day =~ tr/0//;
    }
    $date=`$dateprogram +"%m$day%y"`;
    chomp($date);
} # End of  sub

