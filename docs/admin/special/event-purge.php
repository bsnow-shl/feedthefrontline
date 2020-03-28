<?

$policy = db_multirow('select payment_method_id,expire_after from event_payment_methods where expire_after is not null and expire_after>0');

db_send('begin');
foreach ($policy as $p) {
    $to_expire = db_array("select event_registration_id from event_registrations where payment_method_id=? and paid=0 and confirmed=1 and create_datetime < CURRENT_TIMESTAMP - interval '".(0+$p['expire_after'])." seconds'", $p['payment_method_id']);
    foreach ($to_expire as $eid) {
        db_send('update event_registrations set confirmed=0 where event_registration_id=?', $eid);

        db_insert_hash('event_registration_diary', array(
            'event_registration_id' => $eid, 
            'note' => 'Automatically expired due to non-payment'
            ));

    }
    print_a($to_expire);
}
db_send('commit');
