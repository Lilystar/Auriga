#!/bin/perl

# ��f�B���N�g��
$base_dir = "../../";

# �T�[�o�[�̃t�@�C����
@server_name = ("login-server", "char-server", "map-server");

# �����N�}�b�v����V���{������ǂݍ���
foreach $server( 0.. $#server_name ) {
	push @symbols, symbol_read("$base_dir$server_name[$server].map");
}

# �N���b�V���_���v�̃t�@�C�����J��
open A, "<${base_dir}crashdump.log"         or die "Can't read  crashdump.log";
open B, ">${base_dir}crashdump_report.log" or die "Can't write crashdump_report.log"; 

$server = 0;
while( <A> ) {
	if( /^\t(0x\w+)/ ) {
		# �֐��̃A�h���X�Ȃ̂ŁA�V���{���������߂�
		printf B "$1 : %s\n", sybmol_getname( $symbols[ $server ], hex($1) );
	} elsif( /^\t/ ) {
		# �T�[�o�[�����܂܂�Ă���\��������̂Œ��ׂ�
		my $a;
		foreach $a( 0 .. $#server_name ) {
			if( index($_, $server_name[$a] ) >= 0 ) {
				$server = $a; last;
			}
		}
		print B $_;
	} else {
		# ����ȊO�Ȃ̂ł��̂܂܏o��
		print B $_;
	}
}

close A;
close B;

# �V���{����ǂݍ���
sub symbol_read {
	my $file = shift;
	my @func, @addr, @base;
	open(A, $file) or die "Can't open $file\n";
	while(<A>) {
		if( /(\d+):(\w+)\s+(\w+)H / ) {
			# �L���ȃA�h���X�͈̔�
			$base[ $1 ] = hex($2);
			if( $base[ $1 ] > 0 ) {
				push @addr, [ hex($2), hex($2) + hex($3) ];
			}
		} elsif( /(\d+):(\w+)\s+(\w+)\s+(\w+)/ ) {
			# �֐����Ƃ��̃A�h���X(VC)
			push @func, [ hex($4), $3, $1];
		} elsif( /(\d+):(\w+)\s+(\w+)/ ) {
			# �֐����Ƃ��̃A�h���X
			if( $base[ $1 ] > 0 ) {
				push @func, [ $base[$1] + hex($2), $3, $1 ];
			}
		}
	}
	close A;
	# �A�h���X�����ɑ傫�����ɕ��ёւ���
	@func = sort { $b->[0] <=> $a->[0] } @func;
	return [ \@func, \@addr ];
}

# �A�h���X����֐�����Ԃ�
sub sybmol_getname {
	my $symbol      = shift;
	my $addr        = shift;
	my $symbol_func = $symbol->[0];
	my $symbol_addr = $symbol->[1];
	my $flag = 0;

	# �����ȃA�h���X����
	foreach( @{ $symbol_addr } ) {
		if( $addr >= $_->[0] && $addr <= $_->[1] ) {
			$flag = 1;
		}
	}
	if( !$flag ) {
		# �����ȃA�h���X������
		return "unknown (invalid address)";
	}
	foreach ( @{ $symbol_func } ) {
		if( $addr >= $_->[0] && $_->[1] =~ /^_[^_]/) {
			if( $_->[2] == 1 ) {
				return sprintf "%s + 0x%x", $_->[1], $addr - $_->[0];
			} else {
				return "unknown (invalid type)";
			}
		}
	}
	return "unknown (internal error)";
}

