<?

function dump_table($rows) {
    echo '<table class=styled><tr>';
    $titles = array_keys($rows[0]);
    foreach ($titles AS $t) {
        echo '<th>'.$t.'</th>';
    }
    echo '</tr>';
    foreach($rows AS $r) {
        echo '<tr>';
        foreach ($titles AS $t) {
            echo '<td>'.$r[$t].'</td>';
        }
        echo '</tr>';
    }
    echo '</table>';
}


?>
