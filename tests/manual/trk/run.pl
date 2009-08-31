#!/usr/bin/perl -w

use strict;
use English; # gives us $OSNAME
use File::Temp;
use File::Spec;
use Cwd;

my @ADAPTER_OPTIONS = ();
my @TRKSERVEROPTIONS = ();
my $DUMP_POSTFIX ='.bin';
my $ENDIANESS ='little';

my $isUnix = $OSNAME eq 'linux' ? 1 : 0;
my $MAKE= $isUnix ? 'make' : 'nmake';
my $trkservername;
my $runTrkServer = 1;
my $waitAdapter = 0;

my $usage=<<EOF;
Usage: run.pl -w -av -aq -au -tv -tq -l [COM]
Options:
     -av     Adapter verbose
     -aq     Adapter quiet
     -au     Adapter turn off buffered memory read
     -af     Adapter turn off serial frame
     -w      Wait for termination of Adapter (Bluetooth)
     -tv     TrkServer verbose
     -tq     TrkServer quiet

     trkserver simulator will be run unless COM is specified

Bluetooth:
     rfcomm listen /dev/rfcomm0 1 \$PWD/run.pl -av -af -w {}

EOF

# ------- Parse arguments
my $argCount = scalar(@ARGV);
for (my $i = 0; $i < $argCount; $i++) {
    my $a = $ARGV[$i];
    if ($a =~ /^-/) {
	if ($a eq '-av') {
	    push(@ADAPTER_OPTIONS, '-v');
	} elsif ($a eq '-aq') {
	    push(@ADAPTER_OPTIONS, '-q');
	} elsif ($a eq '-af') {
	    push(@ADAPTER_OPTIONS, '-f');
	} elsif ($a eq '-au') {
	    push(@ADAPTER_OPTIONS, '-u');
	} elsif ($a eq '-w') {
	    $waitAdapter = 1;
	} elsif ($a eq '-tv') {
	    push(@TRKSERVEROPTIONS, '-v');
	} elsif ($a eq '-tq') {
	    push(@TRKSERVEROPTIONS, '-q');
	}  elsif ($a eq '-h') {
	    print $usage;
	    exit(0);
	}  else {
	    print $usage;
	    exit(1);
	}
    } else {
	$trkservername = $a;
	$runTrkServer = 0;
    }
}

# ------- Ensure build is up to date
print "### Building\n";
system($MAKE);
die('Make failed: ' . $!) unless $? == 0;

if ($isUnix != 0) {
    system('killall', '-s', 'USR1', 'adapter', 'trkserver');
    system('killall', 'adapter', 'trkserver');
}

my $userid=$>;
$trkservername = ('TRKSERVER-' . $userid) unless defined $trkservername;
my $gdbserverip= '127.0.0.1';
my $gdbserverport= 2222 + $userid;

print "Serverport: $gdbserverport\n" ;

system('fuser', '-n', 'tcp', '-k', $gdbserverport) if ($isUnix);

# Who writes that?
my $tempFile = File::Spec->catfile(File::Temp::tempdir(), $trkservername);
unlink ($tempFile) if  -f $tempFile;

# ------- Launch trkserver
if ($runTrkServer) {
    my $MEMORYDUMP='TrkDump-78-6a-40-00' . $DUMP_POSTFIX;
    my @ADDITIONAL_DUMPS=('0x00402000' . $DUMP_POSTFIX, '0x786a4000' . $DUMP_POSTFIX, '0x00600000' . $DUMP_POSTFIX);

    my @TRKSERVER_ARGS;
    if ($isUnix) {
	push(@TRKSERVER_ARGS, File::Spec->catfile(File::Spec->curdir(), 'trkserver'));
    } else {
	push(@TRKSERVER_ARGS, 'cmd.exe', '/c', 'start', File::Spec->catfile(File::Spec->curdir(), 'debug', 'trkserver.exe'));
    }

    push(@TRKSERVER_ARGS, @TRKSERVEROPTIONS, $trkservername, $MEMORYDUMP, @ADDITIONAL_DUMPS);

    print '### Executing: ', join(' ', @TRKSERVER_ARGS), "\n";
    my $trkserverpid = fork();
    if ($trkserverpid == 0) {
	exec(@TRKSERVER_ARGS);
	exit(0);
    }

    push(@ADAPTER_OPTIONS, '-s');

    sleep(1);
}

# ------- Launch adapter
my @ADAPTER_ARGS;
if ($isUnix) {
    push(@ADAPTER_ARGS, File::Spec->catfile(File::Spec->curdir(), 'adapter'));
} else {
    push(@ADAPTER_ARGS, 'cmd.exe', '/c', 'start', File::Spec->catfile(File::Spec->curdir(), 'debug', 'adapter.exe'));
}

push(@ADAPTER_ARGS, @ADAPTER_OPTIONS, $trkservername, $gdbserverip . ':' . $gdbserverport);
print '### Executing: ', join(' ', @ADAPTER_ARGS), "\n";
my $adapterpid=fork();
if ($adapterpid == 0) {
    exec(@ADAPTER_ARGS);
    exit(0);
}
die ('Unable to launch adapter') if $adapterpid == -1;

if ($waitAdapter > 0) {
    print '### kill -USR1 ',$adapterpid,"\n";    
    waitpid($adapterpid, 0);
}    
# ------- Write out .gdbinit
my $gdbInit = <<EOF;
# This is generated. Changes will be lost.
#set remote noack-packet on
set confirm off
set endian $ENDIANESS
#set debug remote 1
#target remote $gdbserverip:$gdbserverport
target extended-remote $gdbserverip:$gdbserverport
#file filebrowseapp.sym
add-symbol-file filebrowseapp.sym 0x786A4000
symbol-file filebrowseapp.sym
print E32Main
break E32Main
#continue
#info files
#file filebrowseapp.sym -readnow
#add-symbol-file filebrowseapp.sym 0x786A4000
EOF

my $gdbInitFile = '.gdbinit';
print '### Writing: ',$gdbInitFile, "\n";
open (GDB_INIT, '>' . $gdbInitFile) or die ('Cannot write to ' . $gdbInitFile . ' ' . $!);
print GDB_INIT $gdbInit;
close( GDB_INIT);
