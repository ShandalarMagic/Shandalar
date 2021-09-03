use strict;
use warnings;
use File::Copy;

# Open ManalinkEh.asm and add in the new card / function
my $new_function = $ARGV[0];
if( $new_function ){
my $skip = 0;
$new_function  = "_card_$new_function";
my $extern = 0;
my $jmp = 0;
my $hex;
open my $fh, '<', 'ManalinkEh.asm';
my @lines;
while( my $line = <$fh>){
    if( $line =~ $new_function and not $skip ){
        print "You already added that card.\n";
        $skip = 1;
    }
    if( $line =~ /extern/ ){ $extern = 1; }
    if( $extern and $line !~ /extern/ and not $skip){
        push @lines, "extern $new_function\n"; 
        $extern = 0;
    }
    if( $line =~ /jmp near/ ){ 
        $jmp = 1; 
        ($hex) =  $line =~ /;\s+([A-Z0-9]+)/i;
    }
    if( $jmp and $line !~ /jmp near/ and not $skip){
        my $a = eval( "0x$hex" ) + 5;
        my $new_hex = sprintf "%x", $a;
        push @lines, "  jmp near $new_function     ; $new_hex\n"; 
        $jmp = 0;
        print "Adding $new_function at $new_hex\n";
    }
    push @lines, $line;
}
close $fh;

# Re-write the file
open $fh, '>', 'ManalinkEh.asm';
print { $fh } @lines;
close $fh;

}

# Run make
`make`;

# Copy the new dll to the magic directory
# copy('ManalinkEh.dll','c:\magic\ManalinkEh.dll') or die "Copy failed: $!";
copy('ManalinkEh.dll','c:\magic2k\ManalinkEh.dll') or die "Copy failed: $!";
copy('ManalinkEh.dll','c:\magic2k\zips\ManalinkEh.dll') or die "Copy failed: $!";

#clean up
#`make clean`;