#!/usr/bin/perl

#=========================================================================
# serverstatus.cgi  ver.1.02  by �Ӓ���
#	checkversion�����b�v�����A�T�[�o�[��Ԃ�\������cgi
#
# ** �ݒ���@ **
#
# - ���� $checkv �ϐ��� checkversion �ւ̃p�X��ݒ肷�邱�ƁB
#
# - perl �Ƀp�X���ʂ��Ă��Ȃ��ꍇ�� $perl �� perl �ւ̐������p�X�ɂ��邱�ƁB
#
# - %servers �����ꂼ�ꐳ�����ݒ肷��BIP �̓z�X�g���B
#
# - %servers �Őݒ肵���L���b�V���t�@�C��(��̃t�@�C��)���쐬����B
#   �܂��A���������\�ɂ��邱�ƁB
#   ��> touch ss_login.cache ; chmod 666 ss_login.cache
#
# - �K�v�Ȃ�L���b�V���Ԋu��^�C�g����ύX
#
# - ����map�I�ɂ��Ă���ꍇ��,%servers�ɍs�������邱�ƂőΉ��\�����A
#   @serverorder��%servers�̃L�[�������邱�Ƃ�Y��Ȃ��悤�ɁB
#
# - @state1,@state2�ŕ\���𑽏��ύX�\�B
#
# - ���͕��ʂ�CGI�Ɠ����B�i���s����cgi-bin�t�H���_�Ȃǁj
#
#
# ** ���̑� **
#
# - �����܂Ō��݂̏�ԕ\���Ń��O�͂Ƃ��Ă��Ȃ��̂ŉߋ��̃f�[�^�͎Q�Ƃł��Ȃ�
#
# - �L���b�V���Ԋu�� 0 �ɂ���ƃL���b�V�����Ȃ��Ȃ邪�A
#   ���ׂ����ɑ傫���̂Œ��ӁB
#
# - ping �ɂ��`�F�b�N�͂��܂����x�B�f�t�H���g���ƃ`�F�b�N���Ȃ��B
#   tcp-ping �͕��ׂ������Bicmp-ping �͌y�߂���root�����K�v�Ȃ̂Ŏ��������B
#   Net::Ping �K�{�BActivePerl �Ȃǂł� alarm ���������œ����Ȃ��\������B
#   ���܂������Ȃ��Ȃ� ping ���Ȃ��ق������ׂ��Ⴍ�Ȃ�B
#
#
# ** �G���[�ւ̑Ώ� **
#
# - Can't execute checkversion
#   checkversion ���������A�p�X���Ⴄ�B�������� perl �̃R�}���h�����Ⴄ�B
#
# - Cache file open error.
#   �L���b�V���t�@�C�����J���Ȃ��B�����ƍ쐬���ď������݉\�ɂ��邱�ƁB
#
# - flock error.
#   flock �����T�|�[�g�B�L���b�V�����Ȃ���Έꉞ���������ׂ������B
#   flock ���Ă�ł���Ƃ�����R�����g������Έꉞ�������L���b�V����
#   ���₷���B
#
#-------------------------------------------------------------------------


my($checkv)="../checkversion";	# checkversion �̃p�X(�����炭�ύX���K�v)
my($perl)  ="perl";				# perl �̃R�}���h��

my($checkping)="";			# NG �̂Ƃ� ping �ɂ��`�F�b�N���s��ping�̎��
							# "tcp", "udp", "icmp"(root���K�v), ""����I��
							# ""���� ping ���Ȃ��B
							# Net::Ping ���Ȃ�/��������Ă��Ȃ��Ɩ���

my($cacheperiod) = 120;		# �L���b�V���Ԋu(�b��)

my($title) = "Athena Server Status";	# �y�[�W�^�C�g��

my(@serverorder) = (			# �\����
	"login","char","map"
);
my(%servers) = (				# �f�[�^(ip,���O,�L���b�V���t�@�C����)
	"login"	=> {
					"ip"	=> "127.0.0.1:6900",
					"desc"	=> "Login Server",
					"cache"	=> "./ss_login.cache",
				},
				
	"char"	=> {
					"ip"	=> "127.0.0.1:6121",
					"desc"	=> "Character Server",
					"cache" => "./ss_char.cache",
				},
				
	"map"	=> {
					"ip"	=> "127.0.0.1:5121",
					"desc"	=> "Map Server",
					"cache"	=> "./ss_map.cache",
				},
);

my(@state1) = (					# ��ԕ\��
	"Down",		# �ڑ��ł��Ȃ�
	"Good",		# ���퓮�쒆
	"Error",	# %servers�̐ݒ肪��������(�|�[�g�ԍ�)
	"Closed",	# ping�ɂ͉���
);
my(@state2) = (					# �F
	"#ffc0c0",	# �ڑ��ł��Ȃ�
	"#c0ffc0",	# ���퓮�쒆
	"#c0c0ff",	# %servers�̐ݒ肪��������(�|�[�g�ԍ�)
	"#ffffc0",	# ping�ɂ͉���
);

#--------------------------- �ݒ肱���܂� --------------------------------




use strict;
eval " use Net::Ping; ";


my($msg) = << "EOD";
<html>
<head><title>$title</title></head>
<body text="black" bgcolor="white" link="blue" vlink="blue" alink="blue">
<h1>$title</h1>
<table border=1>
<tr><th>Server</th><th>Address</th><th>Status</th><th>Version</th></tr>
EOD

my(%langconv) = (
);

my($i);
foreach $i (@serverorder){
	my( $state, $ver ) = CheckServer( $servers{$i} );
	
	$msg.= "<tr bgcolor=\"$state2[$state]\"><td>".$servers{$i}->{desc}.
		"</td><td>".$servers{$i}->{ip}."</td><td>$state1[$state]</td>".
		"<td>$ver</td></tr>\n";
}
$msg.="</table></body></html>\n";

print "Content-type: text/html\n\n$msg";

sub CheckServer
{
	my($server) = shift;
	my($state)  = 0;
	
	if($cacheperiod>0)
	{
		open FH, "+<".$server->{cache} or HttpError("Cache file open error.");
		flock FH,2 or HttpError("flock error");
		my(@cache) = split /,/, <FH>;
		if( time < $cache[0] + $cacheperiod )
		{
			close FH;
			return ( $cache[1], $cache[2] );
		}
	}
	
	open PIPE,"$perl $checkv $server->{ip} |"
		or HttpError("Can't execute checkversion.\n");
	my(@dat)=<PIPE>;
	close PIPE;
	
	if($dat[1]=~m/Athena/ && $dat[2]=~/server/){
		if($dat[2]=~/$i/ ){
			$state=1;
		}else{
			$state=2;
		}
	}elsif($checkping){
		eval { 
			my($p) = Net::Ping->new($checkping);
			my($addr)=$server->{ip};
			$addr=~s/\:\d+$//;
			$state=3 if $p->ping($addr);
			$p->close();
		};
	}

	if($cacheperiod>0)
	{
		seek FH,0,0;
		truncate FH,0;
		print FH join( ",", time, $state, $dat[1] );
		close FH;
	}

	return ($state, $dat[1]);
};

sub LangConv {
	my(@lst)= @_;
	my($a,$b,@out)=();
	foreach $a(@lst){
		foreach $b(keys %langconv){
			$a=~s/$b/$langconv{$b}/g;
			my($rep1)=$1;
			$a=~s/\$1/$rep1/g;
		}
		push @out,$a;
	}
	return @out;
}

sub HttpMsg {
	my($msg)=join("", LangConv(@_));
	$msg=~s/\n/<br>\n/g;
	print LangConv("Content-type: text/html\n\n"),$msg;
	exit;
}

sub HttpError {
	my($msg)=join("", LangConv(@_));
	$msg=~s/\n/<br>\n/g;
	print LangConv("Content-type: text/html\n\n"),$msg;
	exit;
}


