<?php

$diary = array();
$diary[] = "Beginning supplier update on ".`date`;

$targets = db_multirow('select supplier_id,feed_url, feed_type from shopping_suppliers where feed_url is not null and feed_type is not null');

foreach ($targets as $t) {
	$diary[] = "Starting supllier $t[feed_url] ";
	$fh = fopen($t['feed_url'], 'r');
	if ($fh == false) {
		$freak_out++;
		$diary[] = "... fopen failed";
		contineue;
	}

	switch ($t['feed_type']) {
	case 'abso':
		fgets($fh); # throw away first line

		$expected = db_array('select supplier_sku from shopping_products where visible_public=1 and supplier_id=?', $t['supplier_id']);
		while (($line = fgets($fh)) !== false && $counter++<1000) {
			list ($url, $name, $desc, $price, $img) = split("\t", $line);
			preg_match("#images/(.*)-L.jpg#", $img, $matches);
			if ($matches[1] && $product = db_row('select product_id,supplier_sku,cost from shopping_products where supplier_id=? and supplier_sku=?', $t['supplier_id'], $matches[1])) {
				$seen[] = $product['supplier_sku'];
				$product['cost'] /= 100.0;
				if ($product['cost'] != $price) {
					$freak_out ++;
					$diary[] = "Oops! $matches[1] differs in cost; ours=$product[cost] theirs=$price. Cost column updated, but you need to figure out a new price.";
					db_send('update shopping_products set cost_changed=1,cost=? where product_id=?', $price*100, $product['product_id']);
				}
			}
		}

		$diff = array_diff($expected, $seen);
		if (count($diff)) {
			$freak_out ++;
			$diary[] = "Oops! ".count($diff)." sku's are missing from the data feed:";
			$diary[] = join(",", $diff);
		}
			
	}
	$diary [] = "Done with that supplier.\n\n";


}

if ($freak_out) {
	cpf_mail("hey@buymyaffection.com", "Buy My Affection supplier problems", join("\n", $diary));
}

?>
