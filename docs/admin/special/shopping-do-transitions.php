<!--
<title><a href="momoney.jpg">Mo Money, Mo Problems</a></title>
-->

<?php 

require_once 'shopping-local.h';
require_once 'shopping.h';

# be paranoid, there's lots of useful debugging and hacking info here
ob_start();


$transitions = shopping_get_transition_list();

foreach ($transitions as $func => $sql) {
	print "<h3>$func</h3>";
	$result = db_multirow($sql);
	print "Result:";
	print '<ul>';
	foreach ($result as $r) {
		print '<li>Shopcart '.$r['shopcart_id'].' qualifies:<blockquote><pre>';
		if (is_callable($func)) {
			$result = call_user_func($func, $r);
			print "</pre</blockquote>result: $result";
			if ($calls++>100) { exit; }
		} else {
			print "... nope, not written yet.";
		}
	}
	print '</ul>';




}



	ob_end_flush();


?>
