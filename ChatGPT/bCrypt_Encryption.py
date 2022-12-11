import bcrypt

# When a user creates a new account

# Generate a random salt value
salt = bcrypt.gensalt()

# Get the user's password
password = "my_secret_password"

# Hash the password using the salt value
password_hash = bcrypt.hashpw(password, salt)

# Store the password hash and salt value in the database

# When the user logs in

# Retrieve the password hash and salt value from the database

stored_password_hash = "..."
stored_salt = "..."

# Get the user's entered password
entered_password = "my_secret_password"

# Check if the entered password matches the stored password hash
if bcrypt.checkpw(entered_password, stored_password_hash):
    # The passwords match, so the user is authenticated
    print("User is authenticated")
else:
    # The passwords do not match, so the user is not authenticated
    print("User is not authenticated")