

create table address_services (
	address_service_id integer not null primary key,
	short_name text unique,
	pretty_name text,
	url text,
	password text
);
insert into pkeys values ('address_services', 'address_service_id');


-- address services
create table user_addresses(
	address_id integer not null primary key,
	strong_address_id varchar(32) unique,
	
	-- addresses can be owned by a user, or by a session (but not both)
	user_id integer references users,
	session_id varchar(32), -- explicitly avoiding doing a 'references' here

	-- delete by setting to 0; we'll sweep through later and clean up
	visible integer default 1 check (visible in (0,1)),

	nickname text not null,
	address_service_id integer references address_services,
	
	-- J=junk; undeliverable
	-- M=marginal; ask for confirmation but delivery anyway
	-- G=good; deliver right away
	quality char not null default 'J',
	last_update timestamp,

	email text,			-- doesn't make much sense when user_id is set

	street text,
	apartment text,
	city text,
	province text,
	country char(2) references countries(iso_code),
	postal_code varchar(20),

	phone text,

	-- e.g. your buzzcode
	delivery_instructions text,

	-- sometimes we'll be able to connect addresses together through the userid;
	-- this will allow updates to be pushed through to the upstream address ID
	flowthru_address_id integer
);
create index user_strong_address on user_addresses(strong_address_id);


