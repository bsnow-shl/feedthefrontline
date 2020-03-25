--
--optional schema for email-verified registration
--


--not_verified contains a verification code sent via email to the 
--  address in 'email'
alter table users add column not_verified text;  --null if email is verified


--optional schema for invite-only registrations
alter table users add column invited_by integer references users;
alter table users add column available_invites integer;
alter table users alter column available_invites set default 0;

--use "not_verified" for invited users by putting invites in the 
--  users table wiht a verification code