insert into roles(role_id,role_name,visible) values (10,'feed_admin',1);

insert into users(user_id, email, password) values (10, 'svanegmond@me.com', 'temp');
insert into users(user_id, email, password) values (11, 'bill.snow@gmail.com', 'temp');

insert into user_role_map(user_id,role_id) values (10,10);
insert into user_role_map(user_id,role_id) values (11,10);

create table hospital_units (
	unit_id integer not null primary key,

	status text default 'new',
	check (status in ('new','accepting','closed')),

	hospital_name text,
	unit_name text,
	city text,
        country text,
	delivery_instructions text,
	food_allergies text,

	contact_email text,
	contact_phone text,

	calendar_token text,
	calendar_iframe_url text,
	calendar_public_url text
);

create table donations (
	donation_id integer not null primary key,

	unit_id integer not null references hospital_units,
	donation_date date,
	timeslot text,

	donor text,

        calendar_token text
);


