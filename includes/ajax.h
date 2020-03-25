<?
global $_cpf_head_magic;

$_cpf_head_magic .= <<<PINKY

<script type="text/javascript" src="/admin/special/js/YAHOO.js" ></script>
<script type="text/javascript" src="/admin/special/js/connection.js" ></script>

<script type="text/javascript">
var dynUpdateHandler = {
	success : function(result) {
			var response = result.responseText;
			var update = new Array();

			if(response.indexOf('|' != -1)) {
				update = response.split('|');
				if (ele = document.getElementById(update[0])) {
					ele.innerHTML = update[1];
				} else {
				}
			} else {
					alert ("Odd: nothing to do" + response);
			}
			return false;
	}


  , failure : function(result) {
						 alert(result.status);
	}
}

function URLEncode( )
{
	// The Javascript escape and unescape functions do not correspond
	// with what browsers actually do...
	var SAFECHARS = "0123456789" +					// Numeric
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ" +	// Alphabetic
					"abcdefghijklmnopqrstuvwxyz" +
					"-_.!~*'()";					// RFC2396 Mark characters
	var HEX = "0123456789ABCDEF";

	var plaintext = document.URLForm.F1.value;
	var encoded = "";
	for (var i = 0; i < plaintext.length; i++ ) {
		var ch = plaintext.charAt(i);
	    if (ch == " ") {
		    encoded += "+";				// x-www-urlencoded, rather than %20
		} else if (SAFECHARS.indexOf(ch) != -1) {
		    encoded += ch;
		} else {
		    var charCode = ch.charCodeAt(0);
			if (charCode > 255) {
			    alert( "Unicode Character " + ch + " cannot be encoded using standard URL encoding. (URL encoding only supports 8-bit characters.) A space (+) will be substituted." );
				encoded += "+";
			} else {
				encoded += "%";
				encoded += HEX.charAt((charCode >> 4) & 0xF);
				encoded += HEX.charAt(charCode & 0xF);
			}
		}
	} // for

	document.URLForm.F2.value = encoded;
	return false;
};

function dynUpdate() {
	action = arguments[0];
	query = '';
	for (i=1; i<arguments.length; i+= 2) {
			query += '&' + arguments[i]+'='+arguments[i+1];	
	}
	if (query) action = action + '?' + query;
	YAHOO.util.Connect.initHeader('X-CPF-Action', action);
	YAHOO.util.Connect.initHeader('Cookie', document.cookie);
	loc = (document.location+'').split('#');
	YAHOO.util.Connect.asyncRequest('GET', loc[0], dynUpdateHandler);
}

</script>

PINKY;

function ajax_action() {
		$pos = strpos($_SERVER['HTTP_X_CPF_ACTION'], '?');
		if ($pos>0) {
				return substr($_SERVER['HTTP_X_CPF_ACTION'], 0, $pos);
		} else {
				return $_SERVER['HTTP_X_CPF_ACTION'];
		}
}

function ajax_parameters() {
		$pos = strpos($_SERVER['HTTP_X_CPF_ACTION'], '?');
		if ($pos>0) {
				$toparse =  substr($_SERVER['HTTP_X_CPF_ACTION'], $pos+1);
				parse_str( $toparse , $rc);
				return $rc;
		} else {
				return array();
		}
}

?>
