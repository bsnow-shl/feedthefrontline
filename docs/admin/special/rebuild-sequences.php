<?

require_once 'dbadmin.h';


$seq = dbadmin_list_sequences();
$tables = dbadmin_list_tables();

if (function_exists('db_local_pkeys')) {
	$pkeys = db_local_pkeys();
} else {
	$pkeys = array();
}
$pkeys += array(
	'upload_id_id' => 'uploads.upload_id',
	'shopping_shopcart_entries_id' => 'shopping_shopcart_entries.shopcart_entry_id',
	'shopping_product_types_id' => 'shopping_product_types.product_type_id',
	'shopping_categories_id' => 'shopping_categories.category_id',
	'i18n_object_id_id' => 'i18n.object_id',
	'error_id_id' => 'errors.error_id',
	'user_id_seq' => 'users.user_id'
);

foreach ($seq as $s) {
    $note = 'ignored';

    $cur = db_value('select nextval(?)', $s)-1;
    db_send('select setval(?,?)', $s, $cur);

    $value = null;
    $sql = '';

    if ($cur  && substr($s, -3, 3) == '_id' || $pkeys[$s]) {
	$try = array();

	if ($pkeys[$s]) {
		$maybe_table_name = substr($pkeys[$s], 0, strpos($pkeys[$s],'.') );
		$try[] = substr($pkeys[$s], strpos($pkeys[$s],'.')+1 );
		$note = "Looking in table $maybe_table_name";
	} else {	
		$maybe_table_name = substr($s, 0, -3);
		$note = "Tried looking in table $maybe_table_name";
	}

        $try[] =  $maybe_table_name."_id";
	$try[] =  $maybe_table_name."_id";
	$try[] = substr($maybe_table_name, 0, -1)."_id";
        $pos = strrpos($maybe_table_name, '_');
        if ($pos) {
            $try []  = substr($maybe_table_name, $pos+1)."_id";
            $try []  = substr($maybe_table_name, $pos+1,-1)."_id";
            $try []  = substr($maybe_table_name, $pos+1,-2)."_id";
        }
   
        foreach ($try as $col) {
            $value = @db_value("select max($col) from $maybe_table_name");
            if ($value!==null) {
                if ($value != $cur) {
                    $note = "max($maybe_table_name.$col) = $value";
                    $sql = sprintf("select setval('%s', $value)", $s, $value);
                } else {
                    $note = 'ok';
                }
            }
        }
    }

        
    $results[] = array(
        'seq_name' => $s,
        'seq_value' => $cur,
        'note' => $note,
        'sql' => $sql,
    );
    if ($sql && $_GET['doit']) {
        db_send($sql);
    }
}

?>

<table cellpadding=4>
<tr><th>sequence</th><th>value</th><th>note</th><th>recommended sql</th></tr>
<? foreach ($results as $r): ?>
<tr>
<Td><%= $r['seq_name'] %></td>
<td><%= $r['seq_value'] %></td>
<td><%= $r['note'] %></td>
<td><%= $r['sql'] %></td>
</tr>
<? endforeach  ?>
</table>
