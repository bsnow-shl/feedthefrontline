update admin_areas set admin_category_id=
	(select admin_category_id from admin_categories where name='Site content');

update admin_areas 
	set admin_category_id=
		(select admin_category_id from admin_categories where name='Back-end') 
	where role_id in (
		select role_id from roles 
		where role_name in ( 
			'user_admin', 
			'role_admin', 
			'error_admin', 
			'paypal_admin', 
			'db_admin', 
			'email_admin'
	));

update admin_areas 
	set admin_category_id=
		(select admin_category_id from admin_categories where name='Site health') 
	where function is not null;
