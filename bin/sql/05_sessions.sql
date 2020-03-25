create table sessions (
	session_id varchar(32),
	session_data text,
	expiry timestamp default CURRENT_TIMESTAMP + interval '6 hours'
);
create unique index session_idx on sessions(session_id);

create function update_session_expiry() returns trigger as '
	
	begin
		NEW.expiry := CURRENT_TIMESTAMP + interval ''6 hours'';
		return NEW;
	end;
' language 'plpgsql';

create trigger expiry_update 
	before update on sessions for each row execute procedure update_session_expiry();

