<?php
// ladmin class by Yor (inspired from a work of MagicalTux for Nezumi)
// functions:
//   ladmin->connect(ip, password, [port]);
//   ladmin->make_account($accname,$pass,$sex);
//   ladmin->make_account2($accname,$pass,$sex,$email);
//   ladmin->delete_account($accname);
//   ladmin->close();
//   ladmin->get_version();
//      answer in a $array:
//        $array['string'] - complete/detailled version
//        $array['ATHENA_MAJOR_VERSION']
//        $array['ATHENA_MINOR_VERSION']
//        $array['ATHENA_REVISION']
//        $array['ATHENA_RELEASE_FLAG']
//        $array['ATHENA_OFFICIAL_FLAG']
//        $array['ATHENA_SERVER_LOGIN']
//        $array['ATHENA_MOD_VERSION']
//   ladmin->accountinfo($id); -- based on list of account
//      answer in a $array:
//        $array['id']
//        $array['accname']
//        $array['sex']
//        $array['dummy'] - not used
//        $array['logincount']
//        $array['state']
//   ladmin->accountinfo2($id); -- based on an specifical packet (by id)
//      answer in a $array:
//        $array['flag'] - always 0
//        $array['id']
//        $array['GM_level']
//        $array['accname']
//        $array['sex']
//        $array['logincount']
//        $array['state']
//        $array['last_ip'] - void in TXT version
//        $array['last_login']
//        $array['email']
//   ladmin->accountinfo3($accname); -- based on an specifical packet (by name)
//      answer in a $array:
//        $array['flag'] - always 0
//        $array['id']
//        $array['GM_level']
//        $array['accname']
//        $array['sex']
//        $array['logincount']
//        $array['state']
//        $array['last_ip'] - void in TXT version
//        $array['last_login']
//        $array['email']
//   ladmin->changepass($accname, $newpass);
//   ladmin->changestate($accname, $newstate);
//   ladmin->checkaccount($accname, $pass);
// Note: all functions return 'false' if an error occurs or if action is not executed.

class ladmin {

	var $sock = false;

	// connect(ip, password, [port]);
	function connect($ip,$admin,$port=6900) {
		// open socket
		$sock=@fsockopen($ip,$port,$errno,$errstr,10);
		if (!$sock) return false;
		// ask for encoding key
		if (fwrite($sock,pack("v",0x791a))==false) {
			fclose($sock);
			return false;
		}
		// get encoding key
		$buf=fread($sock,4); // get 4 bytes
		$buf=unpack("v2val",$buf);
		$len=$buf['val2']-4;
		$buf=$buf['val1'];
		if (($buf!=0x01dc) or ($len<48) or ($len>63)) {
			fclose($sock);
			return false;
		}
		$buf=fread($sock,$len);
		// send encrypted password
		$md5=md5($admin.$buf);
		$packet=pack("v3H32",0x7918,22,2,$md5);
		if (fwrite($sock,$packet)==false) {
			fclose($sock);
			return false;
		}
		// read answer
		$res=fread($sock,3);
		if ($res!="\x19\x79\x00") { // failed !
			fclose($sock);
			return false;
		}
		// save socket pointer
		$this->sock=$sock;
		// return connection done
		return true;
	}

	function make_account($accname,$pass,$sex) {
		// do we have connection?
		if (!$this->sock) return false;
		// check values
		$sex=strtoupper($sex);
		if ( ($sex!='F') and ($sex!='M') ) return false;
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		if ((strlen($pass)<4) or (strlen($pass)>24)) return false;
		// send creation packet
		$packet=pack('v2a24a24a1',0x7930,53,$accname,$pass,$sex);
		if (fwrite($this->sock,$packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$res=fread($this->sock,2);
		if ($res!="\x31\x79") {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		$dat=fread($this->sock,26);
		$buf=unpack("vval/a24name",$dat);
		if ($buf['val']!=0) return false;
		return true;
	}

	function make_account2($accname,$pass,$sex,$email) {
		// do we have connection?
		if (!$this->sock) return false;
		// check values
		$sex=strtoupper($sex);
		if ( ($sex!='F') and ($sex!='M') ) return false;
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		if ((strlen($pass)<4) or (strlen($pass)>24)) return false;
		// send creation packet
		if(strlen($email)>0){
			$packet=pack('v2a24a24a1a40',0x7930,93,$accname,$pass,$sex,$email);
		}else{
			$packet=pack('v2a24a24a1',0x7930,53,$accname,$pass,$sex);
		}
		if (fwrite($this->sock,$packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$res=fread($this->sock,2);
		if ($res!="\x31\x79") {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		$dat=fread($this->sock,26);
		$buf=unpack("vval/a24name",$dat);
		if ($buf['val']!=0) return false;
		return true;
	}

	function delete_account($accname) {
		// do we have connection?
		if (!$this->sock) return false;
		// check values
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		// send deletion packet
		$packet = pack('vva24', 0x7932, 28, $accname);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$res = fread($this->sock, 28);
		$dat = unpack('vpacket/vflag/a24accname', $res);
		if ($dat['packet'] != 0x7933) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		if ($dat['flag']!=0) return false;
		return true;
	}

	function close() {
		// do we have connection?
		if (!$this->sock) return false;
		if (fwrite($this->sock,pack('v',0x7532))==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		fclose($this->sock);
		$this->sock=false;
		return true;
	}

	function get_version() {
		// do we have connection?
		if (!$this->sock) return false;
		// ask for athena version
		if (fwrite($this->sock, pack('v',0x7530))==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// check answer
		$answer=fread($this->sock, 2);
		if ($answer!="\x31\x75") {
			fclose($this->sock);
			$this->sock = false;
			return false;
		}
		// read answer
		$data = fread($this->sock, 8);
		$data = unpack('cATHENA_MAJOR_VERSION/cATHENA_MINOR_VERSION/cATHENA_REVISION/cATHENA_RELEASE_FLAG/cATHENA_OFFICIAL_FLAG/cATHENA_SERVER_LOGIN/vATHENA_MOD_VERSION',$data);
		// create string
		$data['string']='Athena version '.($data['ATHENA_RELEASE_FLAG']?'dev':'stable').'-'.$data['ATHENA_MAJOR_VERSION'].'.'.$data['ATHENA_MINOR_VERSION'];
		if ($data['ATHENA_REVISION']) $data['string'].='r'.$data['ATHENA_REVISION'];
		if ($data['ATHENA_OFFICIAL_FLAG']) $data['string'].='-mod'.$data['ATHENA_MOD_VERSION'];
		return $data;
	}

	function accountinfo($id) { // based on list of account
		// do we have connection?
		if (!$this->sock) return false;
		// check id
		if ($id<0) return false;
		// send account list packet
		$packet = pack('vVVc', 0x7920, $id, $id, 0);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$buf=fread($this->sock,4); // get 4 bytes
		$buf=unpack("v2val",$buf);
		$len=$buf['val2']-4;
		$buf=$buf['val1'];
		if ($buf!=0x7921) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		if ($len!=61) { // account not found
			return false;
		}
		$res = fread($this->sock, 61);
		$dat = unpack('Vid/a24accname/csex/a24dummy/Vlogincount/vstate', $res);
		if ($dat['accname'] === '') return false;
		return $dat;
	}

	function accountinfo2($id) { // based on a specifical packet (by id)
		// do we have connection?
		if (!$this->sock) return false;
		// check id
		if ($id<0) return false;
		// send account list packet
		$packet = pack('vV', 0x793d, $id);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$buf=fread($this->sock,2);
		if ($buf!="\x3e\x79") {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		$res = fread($this->sock, 118);
		$dat = unpack('vflag/Vid/aGM_level/a24accname/csex/Vlogincount/vstate/a16last_ip/a24last_login/a40email', $res);
		$dat['GM_level'] = ord($dat['GM_level']);
		if ($dat['flag'] != 0) return false;
		return $dat;
	}

	function accountinfo3($accname) { // based on a specifical packet (by name)
		// do we have connection?
		if (!$this->sock) return false;
		// check account name
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		// send account list packet
		$packet = pack('va24', 0x793c, $accname);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$buf=fread($this->sock,2);
		if ($buf!="\x3e\x79") {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		$res = fread($this->sock, 118);
		$dat = unpack('vflag/Vid/aGM_level/a24accname/csex/Vlogincount/vstate/a16last_ip/a24last_login/a40email', $res);
		$dat['GM_level'] = ord($dat['GM_level']);
		if ($dat['flag'] != 0) return false;
		return $dat;
	}

	function changepass($accname, $newpass) {
		// do we have connection?
		if (!$this->sock) return false;
		// check values
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		if ((strlen($newpass)<4) or (strlen($newpass)>24)) return false;
		// send password change packet
		$packet = pack('vva24a24', 0x7934, 52, $accname, $newpass);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$res = fread($this->sock, 28);
		$dat = unpack('vpacket/vflag/a24accname', $res);
		if ($dat['packet'] != 0x7935) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		if ($dat['flag']!=0) return false;
		return true;
	}

	function changestate($accname, $newstate) {
		// do we have connection?
		if (!$this->sock) return false;
		// check values
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		// send password change packet
		$packet = pack('vva24V', 0x7936, 32, $accname, $newstate);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$res = fread($this->sock, 32);
		$dat = unpack('vpacket/vflag/a24accname/Vstate', $res);
		if ($dat['packet'] != 0x7937) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		if ($dat['flag']==-1) return false;
		return $dat['state'];
	}

	function checkaccount($accname, $pass) {
		// do we have connection?
		if (!$this->sock) return false;
		// check values
		if ((strlen($accname)<4) or (strlen($accname)>24)) return false;
		if ((strlen($pass)<4) or (strlen($pass)>24)) return false;
		// send password test packet
		$packet = pack('va24a24', 0x793a, $accname, $pass);
		if (fwrite($this->sock, $packet)==false) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		// get answer
		$res = fread($this->sock, 28);
		$dat = unpack('vpacket/vflag/a24accname', $res);
		if ($dat['packet'] != 0x793b) {
			fclose($this->sock);
			$this->sock=false;
			return false;
		}
		if ($dat['flag']!=0) return false;
		return true;
	}

}

