create table email_sandbox(
	email_sandbox_id integer not null primary key,
    id text, -- so you can refer to these later
	tstamp timestamp default CURRENT_TIMESTAMP,
	to_addr text,
	subject text,
	extra_headers text,
	message text
);


