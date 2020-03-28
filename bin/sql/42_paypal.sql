

-- this table will describe systems that can collect payments from paypal
-- there will be one entry for each software module on the site that can collect payments
create table paypal_systems (
	paypal_system_id integer not null primary key,
	paypal_system_name text,	-- e.g. 'events', 'memberships'

	test_mode integer default 0, 	-- running on sandbox
	
	paypal_account_id text,		-- the email address you registered on paypal

	get_shipping_address integer not null default 0, -- ask paypal to get a shipping address
		CHECK ( get_shipping_address in (0,1)),

	get_note integer not null default 0,		-- ask paypal to get the customer to put in a note
		CHECK ( get_note in (0,1)),

	currency_code text not null default 'CAD',

	custom_implementation_header text, 	-- for "redirect to paypal" function
	custom_implementation text,


	notification_header text,	-- file to load for delivering notifications
	notification_function text	-- function to call for notifications
);

-- this table logs transactions we've referred to paypal to collect payment for
create table paypal_transactions (
	paypal_transaction_id integer not null primary key,
	paypal_system_id integer references paypal_systems,
	create_datetime timestamp default CURRENT_TIMESTAMP,
	
	user_id integer references users,

	system_reference integer, -- which item in the paypal system this is for 
	gross numeric(8,2) not null,		-- this
						-- plus tax columns (see ALTER TABLEs below)
						-- equals
	amount_expected numeric(8,2) not null, -- how much we expect to get paid
	currency_expected char(3) not null,

	transaction_status text not null default 'unpaid',
	check ( transaction_status 
	        in ('unpaid', 'paid', 'refunded', 'disputed', 'abandoned')),

	reconciled integer not null default 0,
		check (reconciled in (0,1))
);

-- an unstructured log
create table paypal_transaction_diary (
	paypal_transaction_id integer references paypal_transactions,
	diary_timestamp timestamp default CURRENT_TIMESTAMP,

	note text
);

-- a structured log of IPNs from paypal
create table paypal_notifications (
	paypal_notification_id integer not null primary key,
	paypal_transaction_id integer references paypal_transactions,

	-- paypal has a transaction id (and parent, that refunds use to refer to the original purchase)
	paypal_txnid char(17),
	paypal_parent_txnid char(17),

	notification_timestamp timestamp default CURRENT_TIMESTAMP,

	is_authentic integer default 0,
	requires_action integer default 0,

	check (is_authentic in (0,1)),
	check (requires_action in (0,1)),

	amount_received numeric(8,2)
);



-- store key/value pairs for each notification from paypal
create table paypal_notification_data (
	paypal_notification_id integer not null,

	key text,
	value text 
);


insert into roles (role_name) values ('paypal_admin');

insert into admin_areas(uri,role_id,description)
        values ('/admin/paypal/',  (select role_id from roles where role_name='paypal_admin'), 'PayPal payments');

insert into admin_areas(uri,role_id,description, include_header, function, order_in_category)
        select null, role_id, null , 'paypal.h', 'paypal_render_adminmenu' , 100 from roles where role_name='paypal_admin';



create table tax_schemes (
	tax_scheme_id integer not null primary key,

	tax_scheme_name text
);

create table tax_collection (
	tax_collection_id integer not null,
	tax_scheme_id integer references tax_schemes,
	sql_column text not null ,

	tax_name_en text,
	tax_name_fr text,
	tax_number text,
	tax_amount numeric(4,4)
);




insert into tax_schemes (tax_scheme_id, tax_scheme_name) values (1, 'Quebec');
insert into tax_schemes (tax_scheme_id, tax_scheme_name) values (2, 'Ontario');
insert into tax_schemes (tax_scheme_id, tax_scheme_name) values (3, 'BC');
insert into tax_schemes (tax_scheme_id, tax_scheme_name) values (4, 'Alberta');
insert into tax_collection  (tax_collection_id, tax_scheme_id, tax_name_en, tax_number, tax_amount,sql_column)
	select 1, tax_scheme_id,'Ontario PST', '', '0.08','ontario_pst' from tax_schemes where tax_scheme_name='Ontario';
insert into tax_collection  (tax_collection_id, tax_scheme_id, tax_name_en, tax_number, tax_amount,sql_column)
	select 2, tax_scheme_id,'GST', '', '0.06','gst' from tax_schemes where tax_scheme_name='Ontario';
insert into tax_collection  (tax_collection_id, tax_scheme_id, tax_name_en, tax_number, tax_amount,sql_column)
	select 3, tax_scheme_id,'GST', '', '0.06','gst' from tax_schemes where tax_scheme_name='Quebec';
insert into tax_collection  (tax_collection_id, tax_scheme_id, tax_name_en, tax_number, tax_amount,sql_column)
	select 4, tax_scheme_id,'QST', '', '0.075','qst' from tax_schemes where tax_scheme_name='Quebec';
insert into tax_collection  (tax_collection_id, tax_scheme_id, tax_name_en, tax_number, tax_amount,sql_column)
	select 5, tax_scheme_id,'GST', '', '0.06','gst' from tax_schemes where tax_scheme_name='BC';
insert into tax_collection  (tax_collection_id, tax_scheme_id, tax_name_en, tax_number, tax_amount,sql_column)
	select 6, tax_scheme_id,'GST', '', '0.06','gst' from tax_schemes where tax_scheme_name='Alberta';

alter table paypal_transactions add column qst numeric(8,2);
alter table paypal_transactions add column gst numeric(8,2);
alter table paypal_transactions add column ontario_pst numeric(8,2);




insert into i18n_classes (class_id, class_name) values (420, 'Tax scheme');
insert into i18n_fields(field_id,class_id, field_name, pretty_name_en, field_type) values
        (4201, 420, 'description', 'Description', 'text');


