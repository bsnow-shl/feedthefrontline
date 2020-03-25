<?

require_once 'users.h';

function tinyplanet_user_admin_edit_callback($required = 0) {
    return array("", array(
        'company' => 'text',
        'address' => 'text',
        'address_2' => 'text'
    ));
    
}?>
