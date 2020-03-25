create table events (
	event_id integer not null primary key,

	name text,	-- for administrative UI
	registration_start_date date not null default CURRENT_TIMESTAMP,
	registration_end_date   date not null,

	event_start_date        date not null,
	event_end_date          date,            -- only relevant for multi-day events

	max_attendees smallint,
	tax_scheme_id integer references tax_schemes,

	currency_code char(3) not null default 'CAD',

	publishing_status text default 'unpublished',
	check (publishing_status in ('unpublished', 'published', 'archived')),

	allow_waitinglist smallint not null default 0,
	manual_approval smallint not null default 0,
	disable_postal_fields smallint not null default 0,

	-- for outgoing mails
	reply_to text not null default '', 

	custom_registration_form_header text not null default '',
	custom_registration_form_function text not null default ''

	-- actual event content (title, body, description) are stored in 
	-- the publishing framework's i18n tables
);


CREATE TABLE event_pricing_constraints (
    constraint_id integer NOT NULL primary key,
    title text NOT NULL,
    "header" text NOT NULL,
    "function" text NOT NULL,
    editing_page text,
    show_to_public integer  not null DEFAULT 1
);
insert into event_pricing_constraints
    values (1,'Always show', 'events.h', 'event_true', null, 1);


create table event_pricing (
	event_pricing_id integer not null primary key,
	event_id integer not null references events,

	name text, 	-- for administrative UI
	external_sku text,

	-- content to describe the price point is stored in i18n tables
	price numeric(10,2) not null,
		CHECK (price>=0),

	-- some event pricepoints may not be available (e.g. depends on membership level)
	-- this callback is described here
	pricing_availability_header text,
	pricing_availability_callback text,

    pricing_constraint_id integer references event_pricing_constraints,
    pricing_constraint_extra text not null default '',


	-- if the price point is unavailable, it can still be shown 
	-- albeing unselectable if this is 1
	show_to_public smallint
);
insert into i18n_classes(class_id, class_name) values (431, 'Event pricing');
insert into i18n_fields(field_id, class_id, field_name, pretty_name_en, field_type) values 
	(4311, 431, 'description', 'Price point description', 'text');

CREATE TABLE event_payment_methods (
    payment_method_id integer not null primary key,
    short_name text unique,
    style text,	-- "clickthrough" or "silent"
    
    header text,
    "function" text,

    nag_if_unpaid smallint default 0
);
insert into i18n_classes(class_id,class_name) values (433, 'Event payment method');
insert into i18n_fields(field_id, class_id, field_name, pretty_name_en, field_type) values 
	(4331, 433, 'description', 'Description', 'text');
insert into i18n_fields(field_id, class_id, field_name, pretty_name_en, field_type) values 
	(4332, 433, 'confirmation', 'Confirmation screen message', 'richtext');


create table event_registrations (
	event_registration_id integer not null primary key,

	-- this user
	user_id integer references users,

	-- or this newcomer
	first_name text,
	last_name text,
	email text,
	telephone text,
	company text,
	address text,
	city text,
	province text,
	country text,
	postal_code text,

	-- registered for this event at this price point
	event_pricing_id integer not null references event_pricing,

	-- 'atdoor', 'paypal', may be set to "freebie" in admin
	payment_method_id integer not null references event_payment_methods,

	-- is the registration confirmed (i.e. are they going)?
	confirmed smallint default 0,
	check (confirmed in (0,1)),

	-- is the registration paid for?
	paid smallint default 0,
	check (paid in (0,1)),
    paid_datetime timestamp,

	-- needs eyeballing?
	needs_approval smallint default 0,
	check (needs_approval in (0,1)),

	create_datetime timestamp default CURRENT_TIMESTAMP,
	email_sent smallint default 0,
	check (email_sent in (0,1))
);




INSERT INTO event_payment_methods (payment_method_id, short_name, style, "function", header ) VALUES (1, 'phone', 'silent', 'event_registration_pay_telephone', 'events.h');
INSERT INTO event_payment_methods (payment_method_id, short_name, style, "function", header ) VALUES (2, 'paypal', 'clickthrough', 'event_registration_pay_paypal', 'events.h');

insert into classes (class_id, class_name,table_name,table_pkey_column,table_pretty_column) values (43, 'Events', 'events', 'event_id', 'name');
insert into roles (role_name) values ('event_manager');
update roles set visible=0,role_parameters='required',role_parameter_class_id=(select class_id from classes where class_name='Events')
	WHERE role_name='event_manager';

insert into roles (role_name) values ('events_admin');
insert into admin_areas(uri,role_id,description)
        select '/admin/events//',  role_id, 'Online event registration' from roles where role_name='events_admin';


insert into admin_areas(role_id,include_header,function) select role_id, 'events.h', 'events_render_adminmenu' from roles where role_name='events_admin';

create table event_registration_diary (
	event_registration_id integer not null references event_registrations,

	diary_timestamp timestamp default CURRENT_TIMESTAMP,
	note text
);


CREATE TABLE event_additional_content (
	event_additional_content_id integer NOT NULL,
	event_id integer NOT NULL,
	short_name text
);
create index event_addl_event_id on event_additional_content(event_id);


insert into i18n_classes(class_id, class_name) values ('430', 'Event');
insert into i18n_fields(field_id,class_id, field_name, pretty_name_en, field_type) values 
	(4301, 430, 'title', 'Title', 'text');
insert into i18n_fields(field_id,class_id, field_name, pretty_name_en, field_type) values 
	(4302, 430, 'description', 'Description', 'richtext');

insert into i18n_classes(class_id, class_name) values ('432', 'Event additional content');
insert into i18n_fields(field_id, class_id, field_name, pretty_name_en, field_type) values 
	(4321, 432, 'title', 'Title', 'text');
insert into i18n_fields(field_id, class_id, field_name, pretty_name_en, field_type) values 
	(4322, 432, 'body', 'Body', 'richtext');



CREATE TABLE event_sessions (
        event_session_id integer NOT NULL primary key,
        event_id integer NOT NULL references events,
        session_start timestamp without time zone NOT NULL,
        duration integer NOT NULL
);


CREATE TABLE event_session_options (
        session_option_id integer NOT NULL primary key,
        event_session_id integer NOT NULL references event_sessions,
        capacity integer NOT NULL
);

CREATE TABLE event_session_registrations (
	event_registration_id integer NOT NULL references event_registrations,
	session_option_id integer NOT NULL references event_session_options
);

