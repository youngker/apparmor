#! /usr/bin/perl -w
#
# mkprofile.pl -
#   generate a formatted profile based on passed in arguments
#
# Gawd, I hate writing perl. It shows, too.
#
my $__VERSION__=$0;

use strict;
use Getopt::Long;

my $help = '';
my $nowarn = '';
my $escape = '';
my %output_rules;
my $hat = "__no_hat";
my %flags;

GetOptions(
  'escape|E' => \$escape,
  'nowarn' => \$nowarn,
  'help|h' => \$help,
);

sub usage {
  print STDERR "$__VERSION__\n";
  print STDERR "Usage $0 [--nowarn|--escape] execname [rules]\n";
  print STDERR "      $0 --help\n";
  print STDERR "  nowarn:      don't warn if execname does not exist\n";
  print STDERR "  escape:      escape stuff that would be treated as regexs\n";
  print STDERR "  help:        print this message\n";
}

&usage && exit 0 if ($help || @ARGV < 1);

sub gen_netdomain($) {
  my $rule = shift;
  # only split on single ':'s
  my @rules = split (/(?<!:):(?!:)/, $rule);
  # convert '::' to ':' -- for port designations
  foreach (@rules) { s/::/:/g; }
  push (@{$output_rules{$hat}}, "  @rules,\n");
}

sub gen_network($) {
  my $rule = shift;
  my @rules = split (/:/, $rule);
  push (@{$output_rules{$hat}}, "  @rules,\n");
}

sub gen_cap($) {
  my $rule = shift;
  my @rules = split (/:/, $rule);
  if (@rules != 2) {
    (!$nowarn) && print STDERR "Warning: invalid capability description '$rule', ignored\n";
  } else {
    push (@{$output_rules{$hat}}, "  capability $rules[1],\n");
  }
}

sub gen_file($) {
  my $rule = shift;
  my @rules = split (/:/, $rule);
  # default: file rules
  if (@rules != 2) {
    (!$nowarn) && print STDERR "Warning: invalid file access '$rule', ignored\n";
  } else {
    if ($escape) {
      $rules[0]=~ s/(["[\]{}\\\:\#])/\\$1/g;
      $rules[0]=~ s/(\#)/\\043/g;
    }
    if ($rules[0]=~ /[\s\!\"\^]/) {
      push (@{$output_rules{$hat}}, "  \"$rules[0]\" $rules[1],\n");
    } else {
      push (@{$output_rules{$hat}}, "  $rules[0] $rules[1],\n");
    }
  }
}

sub gen_flag($) {
  my $rule = shift;
  my @rules = split (/:/, $rule);
  if (@rules != 2) {
    (!$nowarn) && print STDERR "Warning: invalid flag description '$rule', ignored\n";
  } else {
    push (@{$flags{$hat}},$rules[1]);
  }
}

sub gen_hat($) {
  my $rule = shift;
  my @rules = split (/:/, $rule);
  if (@rules != 2) {
    (!$nowarn) && print STDERR "Warning: invalid hat description '$rule', ignored\n";
  } else {
    $hat = $rules[1];
    # give every profile/hat access to change_hat
    @{$output_rules{$hat}} = ( "  /proc/*/attr/current w,\n",);
  }
}

my $bin = shift @ARGV;
!(-e $bin || $nowarn) && print STDERR "Warning: execname '$bin': no such file or directory\n";

# give every profile/hat access to change_hat
gen_file("/proc/*/attr/current:w");

for my $rule (@ARGV) {
  #($fn, @rules) = split (/:/, $rule);
  if ($rule =~ /^(tcp|udp)/) {
    # netdomain rules
    gen_netdomain($rule);
  } elsif ($rule =~ /^network:/) {
    gen_network($rule);
  } elsif ($rule =~ /^cap:/) {
    gen_cap($rule);
  } elsif ($rule =~ /^flag:/) {
    gen_flag($rule);
  } elsif ($rule =~ /^hat:/) {
    gen_hat($rule);
  } else {
    gen_file($rule);
  }
}

sub emit_flags($) {
  my $hat = shift;

  if (exists $flags{$hat}) {
    print STDOUT " flags=(";
    print STDOUT pop(@{$flags{$hat}});
    foreach my $flag (@{$flags{$hat}}) {
      print STDOUT ", $flag";
    }
    print STDOUT ") ";
  }
}

print STDOUT "# Profile autogenerated by $__VERSION__\n";
print STDOUT "$bin ";
emit_flags('__no_hat');
print STDOUT "{\n";
foreach my $outrule (@{$output_rules{'__no_hat'}}) {
  print STDOUT $outrule;
}
foreach my $hat (keys %output_rules) {
  if (not $hat =~ /^__no_hat$/) {
    print STDOUT "\n  ^$hat";
    emit_flags($hat);
    print STDOUT " {\n";
    foreach my $outrule (@{$output_rules{$hat}}) {
      print STDOUT "  $outrule";
    }
    print STDOUT "  }\n";
  }
}
#foreach my $hat keys
#foreach my $outrule (@output_rules) {
#  print STDOUT $outrule;
#}
print STDOUT "}\n";
