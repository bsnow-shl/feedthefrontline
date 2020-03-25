

-- okay, whose products do we carry?
create table shopping_suppliers (
	supplier_id integer not null primary key,
	short_name text unique,

	visible integer,
	check (visible in (0,1)),

	-- which contract this supplier is subject to
	contract_id integer references contracts,

	feed_type text,
	feed_url text,

	pricing_rule_header text,
	pricing_rule_function text
);


create table shopping_product_types (
	product_type_id integer not null primary key,
	
	short_name text unique,

	fulfillment_header text,
	fulfillment_function text,

	commission_header text,
	commission_function text
);

-- some products can be customized.
-- greeting cards have a gift message
-- mix cd's have a track list
-- lingerie has a size that is entered by the *recipient*
-- flowers can be small, medium, or large
create table shopping_product_type_options (
	type_option_id integer not null primary key,

	product_type_id integer not null references shopping_product_types,

	-- so product "VS-1" lets you pick bra size.
	option_name text not null,

	-- text,bigtext,menu
	option_type text not null,

	option_enterer text not null,
	check (option_enterer in ('recipient', 'sender', 'system', 'fulfiller'))

	-- pretty name is in i18n
);
create index spto_ptid on shopping_product_type_options(product_type_id);


create table shopping_product_type_option_choices (
	option_choice_id integer not null primary key,

	type_option_id integer not null references shopping_product_type_options,
	display_order integer,

	choice_name text not null,
	price_premium integer,
	cost_premium integer

	-- pretty stuff is in 18in
);
create index sptoc_toid on shopping_product_type_option_choices(type_option_id);




-- and what are those products?
create table shopping_products (
	product_id integer not null primary key,
	create_date timestamp default CURRENT_TIMESTAMP,

	supplier_id integer not null references shopping_suppliers,
	product_type_id integer references shopping_product_types,

	visible_public integer not null default 1
		check(visible_public in (0,1)),
	
	visible_admin integer not null default 1
		check(visible_admin in (0,1)),

	check ( not (visible_public=1 and visible_admin=0)),

	-- used to determine product imagery
	sku text,
	supplier_sku text,



	-- title, description in i18n
	
	price integer,
	cost integer,
	cost_changed integer not null default 0,
	check(cost_changed in (0,1))

);
create index sp_sid on shopping_products(supplier_id);
create index sp_ptid on shopping_products(product_type_id);
create index sp_sku on shopping_products(sku);


-- and what categories are they in?
create table shopping_categories (
	category_id integer not null primary key,

	short_name text unique
	
	-- details in i18n
);
create index sc_short_name on shopping_categories(short_name);



create table shopping_category_category_map (
	category_id integer not null references shopping_categories,
	
	-- null means top level
	parent_category_id integer references shopping_categories(category_id),
	display_order integer

);
create index ssccm_pcid on shopping_category_category_map(parent_category_id);


create table shopping_product_category_map (
	category_id integer references shopping_categories,

	product_id integer references shopping_products,
	display_order integer
);
create index spcm_pid on shopping_product_category_map(product_id);
create index spcm_cid on shopping_product_category_map(category_id);



-- and now information entered at runtime


-- list of shopping carts
create table shopping_shopcarts (
	shopcart_id integer not null primary key,
	strong_shopcart_id varchar(32),

	status varchar(30) default 'new',
	paid integer default 0,
	check (paid in (0,1)),
	needs_attention integer default 0,
	is_test_order integer default 0,

	created_tstamp timestamp default CURRENT_TIMESTAMP,
	last_update timestamp default CURRENT_TIMESTAMP,

	-- various reminder timestamps
	next_finalized_reminder timestamp,
	finalized_reminder_count integer,

	next_address_reminder timestamp,
	address_reminder_count integer,

	-- summary, not to be shown to the user
	cached_price integer,

	-- ownership information (1 will do)
	user_id integer references users,
	session_id varchar(32),

	buyer_email text,
	buyer_name text,
	buyer_phone text
);

create index ss_uid on shopping_shopcarts(user_id);
create index ss_ssid on shopping_shopcarts(strong_shopcart_id);
create index ss_status on shopping_shopcarts(status);
create index ss_paid on shopping_shopcarts(paid);
create index ss_status_paid on shopping_shopcarts(user_id,paid);
CREATE FUNCTION "shopping_cart_update" ( ) RETURNS trigger AS '
BEGIN
	if (NEW.last_update = OLD.last_update) THEN
		NEW.last_update = CURRENT_TIMESTAMP;
	end if;
	return NEW;
END;
' LANGUAGE 'plpgsql';
create trigger shopping_cart_update_trigger before update on shopping_shopcarts
	for each row execute procedure shopping_cart_update();



-- diary of the life of a shopcart
create table shopping_shopcart_diary (
	shopcart_id integer not null references shopping_shopcarts,
	tstamp timestamp default CURRENT_TIMESTAMP,
	user_id integer references users,
	message text
);
create index ssd_scid on shopping_shopcart_diary(shopcart_id);



-- then what items are in them
create table shopping_shopcart_entries (
	shopcart_entry_id integer not null primary key,

	-- this product
	product_id integer not null references shopping_products,

	-- is in this shopcart
	shopcart_id integer not null references shopping_shopcarts,

	-- destined for this address_id
	address_id integer references user_addresses,

	-- oops! no, they dropped it
	dropped integer default 0

	-- note: no quanity: just repeat the row because there's all those fussy options
);
create index sse_shopcart_id on shopping_shopcart_entries(shopcart_id);



-- and what options they have
create table shopping_shopcart_entry_options (
	shopcart_entry_id integer references shopping_shopcart_entries,

	type_option_id integer not null references shopping_product_type_options,
	value text
);



update countries set visible=0;
update countries set visible=1 where iso_code in ('US', 'CA');
update countries set display_order=100;
update countries set display_order=99 where iso_code='CA';
update countries set display_order=98 where iso_code='US';

insert into i18n_classes(class_id,class_name) values (450,'Product type option');
insert into i18n_fields(field_id,class_id,field_name,pretty_name_en,field_type) values (4501, 450, 'pretty_name', 'Option pretty name', 'text');

insert into i18n_classes(class_id,class_name) values (451,'Product type option choice');
insert into i18n_fields(field_id,class_id,field_name,pretty_name_en,field_type) values (4511, 451, 'pretty_name', 'Choice pretty name', 'text');

insert into i18n_classes(class_id,class_name) values (452,'Product category');
insert into i18n_fields(field_id,class_id,field_name,pretty_name_en,field_type) values (4521, 452, 'title', 'Title', 'text');
insert into i18n_fields(field_id,class_id,field_name,pretty_name_en,field_type) values (4522, 452, 'caption', 'Caption', 'richtext');

insert into i18n_classes(class_id,class_name) values (453,'Product');
insert into i18n_fields(field_id,class_id,field_name,pretty_name_en,field_type) values (4531, 453, 'title', 'Title', 'text');
insert into i18n_fields(field_id,class_id,field_name,pretty_name_en,field_type) values (4532, 453, 'description', 'Description', 'richtext');

insert into i18n_classes (class_id, class_name, editable_in_cms) VALUES (454, 'Product type', 0);

insert into roles (role_name) values ('shopping_admin');
insert into admin_areas(uri,description,role_id) 
        select '/admin/shopping/', 'Shopping admin', role_id from roles where role_name='shopping_admin';


alter table shopping_product_types add column class_id integer;

create table shopping_realms ( 
	realm_id integer not null primary key,
	short_name text unique not null,
	pretty_name text not null,
	uri text not null,
	required_role text
	);
create table shopping_realm_product_map (
	realm_id integer not null references shopping_realms,
	product_id integer not null references shopping_products,
	unique (realm_id,product_id)
);

