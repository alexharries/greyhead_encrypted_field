<?php

/**
 * @file
 * greyhead_encrypted_field.api.inc
 *
 * Functions provided by the greyhead_encrypted_field module.
 */

/**
 * Get the default ACL entries for an encrypted field.
 *
 * ACL entries are specified as: entity:[entity type]|name:[entity identifier]|permissions:[permission 1],[permission 2] (etc)
 *
 * By default, the following entity types are handled:
 *
 *  - user-type: types of user, including:
 *    - creator: the user who created the field and its parent element, e.g.
 *      "user-type:creator"
 *    - root: the primary administration account (UID 1), e.g. "user-type:root"
 *
 *  - user-uid: a user account's UID, e.g. "user-uid:123"
 *
 *  - user-name: a user account's login name, e.g. "user-name:alexharries"
 *
 *  - role-drupal: one of the drupal-provided roles:
 *      anonymous user
 *      authenticated user
 *      administrator
 *
 *    e.g. "role-drupal:authenticated user"
 *
 *  - role-custom: a user role
 */
function greyhead_encrypted_field_acl_default_acls() {
  $default_acls = &drupal_static(__FUNCTION__);

  if (is_null($default_acls)) {
    $default_acls = variable_get('greyhead_encrypted_field_default_acls');

    if (is_null($default_acls)) {
      $default_acls_array = array();

      // Implement a hook_greyhead_encrypted_field_acl_default_acls_alter.
      drupal_alter(GEF_MODULE_NAME . '_acl_default_acls', $default_acls_array);

      // Convert back into strings.
      $default_acls = greyhead_encrypted_field_acl_acls_to_strings($default_acls_array);

      // Set the default ACLs in the variable table.
      variable_set('greyhead_encrypted_field_default_acls', $default_acls);
    }
  }

  return $default_acls;
}

/**
 * Implements hook_greyhead_encrypted_field_acl_default_acls_alter.
 *
 * @param $default_acls
 */
function greyhead_encrypted_field_greyhead_encrypted_field_acl_default_acls_alter(&$default_acls) {
  $acl = new greyhead_acl(GEF_USER_TYPE, GEF_USER_TYPE_CREATOR, GEF_PERMISSION_FULL_CONTROL);
  $default_acls[$acl->to_string()] = $acl;

  $acl = new greyhead_acl(GEF_ROLE_DRUPAL, GEF_ROLE_DRUPAL_ADMINISTRATOR, GEF_PERMISSION_FULL_CONTROL);
  $default_acls[$acl->to_string()] = $acl;

  $acl = new greyhead_acl(GEF_ROLE_DRUPAL, GEF_ROLE_DRUPAL_AUTHENTICATED, GEF_PERMISSION_VIEW_TITLE);
  $default_acls[$acl->to_string()] = $acl;
}

/**
 * Convert an array of ACLs to a textarea-friendly chunk of text.
 *
 * @param array $acls
 *
 * @return string
 */
function greyhead_encrypted_field_acl_acls_to_strings(array $acls) {
  $acl_strings = array();

  if (is_array($acls) && !empty($acls)) {
    foreach ($acls as $acl) {
      $acl_strings[] = $acl->to_string();
    }
  }

  return implode(GEF_PERMISSION_LINE_BREAK, $acl_strings);
}

/**
 * Convert an ACL textfield into an array of ACLs.
 *
 * @param $textfield
 *
 * @return array
 */
function greyhead_encrypted_field_acl_textfield_to_acls($textfield) {
  // Trim the data and normalise the line endings.
  $textfield = trim(greyhead_customisations_normalise_line_endings($textfield));

  // Break the text into ACLs.
  $acl_strings = explode(GEF_PERMISSION_LINE_BREAK, $textfield);

  // Initialise an array of ACL entities.
  $acls = array();

  // Trim each ACL as well.
  foreach ($acl_strings as $acl_string) {
    $acls[$acl_string] = greyhead_encrypted_field_acl_string_to_acl($acl_string);
  }

  return $acls;
}

/**
 * Break an ACL string into an ACL object.
 *
 * For example, the ACL string:
 *
 * 'user-type:creator::permissions:view value,view title,edit value,edit title'
 *
 * ... will return an array such as:
 *
 *  array(
 *    'entity-type' => array(0 => 'user-type'),
 *    'user-type' => array(0 => 'creator'),
 *    'permissions' => array(
 *      0 => 'view value',
 *      1 => 'view title',
 *      2 => 'edit value',
 *      3 => 'edit title',
 *    ),
 *  );
 *
 * @param $acl_string
 *
 * @return array|bool
 */
function greyhead_encrypted_field_acl_string_to_acl($acl_string) {
  // Split the ACL string by its main parts, which will give us two
  // sections - the entity type and entity identifier, and the permissions
  // granted to that entity.
  $acl_components = explode(GEF_PERMISSIONS_PART_SEPARATOR, $acl_string);
  gefdpm(__FUNCTION__);
  gefdpm($acl_components);
  gefdpm($acl_string);

  // The first part will be the entity type and ID.
  list($entity_type, $entity_id) = explode(GEF_PERMISSIONS_KEY_VALUE_SEPARATOR, $acl_components[0], 2);

  // The second part will be the permission(s).
  list($permission_label, $permissions_string) = explode(GEF_PERMISSIONS_KEY_VALUE_SEPARATOR, $acl_components[1], 2);

  return new greyhead_acl($entity_type, $entity_id, $permissions_string);
}

/**
 * Checks whether a user has permission to perform a defined action.
 *
 * For a list of actions available,
 * @see      greyhead_encrypted_field_acl_permissions_available().
 *
 * @param      $permission string The permission being checked, for example 'edit label'
 *                         or 'view value', etc.
 * @param      $acls
 * @param      $entity
 * @param null $entity_type
 * @param null $account
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_user_access($permission, $acls, $entity = NULL, $entity_type = NULL, $account = NULL) {
  // @TODO: statically cache part or all of this.

  // Create our access result, defaulting to no access.
  $access = FALSE;

  // Get the user account if not already provided.
  if (is_null($account)) {
    $account = $GLOBALS['user'];
  }

//  // If $instance is new, $element will be NULL.
//  if (is_null($entity)) {
//    $instance['acls'] = greyhead_encrypted_field_acl_default_acls();
//  }

//  gefdpm(__FUNCTION__ . ' - unfinished');
//  gefdpm($instance);
//  gefdpm($entity);
//  gefdpm($entity_type);
//  gefdpm($permission);
//  gefdpm($account);

  // Check if the user is a super-duper admin.
  $access = $access || greyhead_encrypted_field_acl_user_is_admin($account, $entity_type);

  // Check that the requested permission is a valid permission name.
  if (!$access && greyhead_encrypted_field_acl_valid_permission($permission)) {
    // It's valid. Now, we need to see if any other permissions also grant this
    // permission. For example, "edit value" has the implicit permission "view
    // value".
    $permissions_which_grant_this_permission = greyhead_encrypted_field_acl_permission_also_granted_by($permission);

    // Create an array with all the permissions which will return a TRUE.
    $all_possible_permissions = $permissions_which_grant_this_permission;
    $all_possible_permissions[] = $permission;

    // Loop through each ACL and check whether any of them define any of
    // $all_possible_permissions.
    foreach ($acls as $acl) {
      // Is it a valid ACL?
      if ($acl->validate()) {
        // Yes, its entity type and ID, and permissions, are valid.

        // Does this ACL define one or more of the permissions we're looking
        // for?
        $permissions_defined_by_acl = $acl->defines_permissions($all_possible_permissions);

        if (!empty($permissions_defined_by_acl)) {
          // Yes - this ACL entry provides the permission we're looking for. Check
          // whether the user is a member of the ACL's entity type. If they are,
          // then we can return TRUE.

          // Load the entity type info, which tells us the access callback.
          $acl_entity_type_info = greyhead_encrypted_field_acl_entity_type_info($acl->entity_type);

          // Determine the access function name.
          $access_callback_function = $acl_entity_type_info['access callback'];

          if (function_exists($access_callback_function)) {
            // Assemble the $options array.
            $options = array(
              'property_id' => $acl->entity_id,
              'uid' => $account->uid,
//              'field_instance' => $instance,
              'entity_uid' => isset($entity->uid) ? $entity->uid : NULL,
              'entity' => $entity,
            );

            // At last, we know everything we need to - check if the user has
            // access as a result of this ACL.
            $access = $access || call_user_func($access_callback_function, $options);
          }
        }
      }

      // If permission has been granted, stop processing since we don't need to
      // go any further.
      if ($access) {
        break;
      }
    }
  }

  return $access;
}

/**
 * Get a list of any other permissions which are granted by $permission.
 *
 * For example, the $permission "edit value" implies "view value", because
 * otherwise you wouldn't be able to view the field's value when editing it.
 *
 * @param $permission
 *
 * @return mixed
 */
function greyhead_encrypted_field_acl_permission_also_grants_permissions($permission) {
  if ($permission_data = greyhead_encrypted_field_acl_permissions_available($permission)) {
    return $permission_data['implicit permissions'];
  }

  return FALSE;
}

/**
 * Get a list of any permissions which implicitly grant $permission.
 *
 * For example, if $permission is "view value", then the permissions "edit
 * value", "edit permissions" and "full control" would be returned, because they
 * all imply access to view the value of an encrypted field.
 *
 * @param $permission
 */
function greyhead_encrypted_field_acl_permission_also_granted_by($permission) {
  if ($permission_data = greyhead_encrypted_field_acl_permissions_available($permission)) {
    return $permission_data['also granted by'];
  }

  return FALSE;
}

/**
 * Check that the requested permission is a valid permission name.
 *
 * @param $permission
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_valid_permission($permission, $permissions = NULL) {
  if (is_null($permissions)) {
    $permissions = greyhead_encrypted_field_acl_permissions_available();
  }

  return array_key_exists($permission, $permissions);
}

/**
 * Gets information on validation and access functions for a type of entity.
 *
 * @param $acl_entity_type
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_entity_type_info($acl_entity_type) {
  // Get the list of access providers.
  $acl_entity_types = greyhead_encrypted_field_acl_entity_types_info();

  return isset($acl_entity_types[$acl_entity_type]) ? $acl_entity_types[$acl_entity_type] : FALSE;
}

/**
 * Checks whether a given entity type - such as "user-uid" - is valid.
 *
 * @param $acl_entity_type
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_entity_type_validate($acl_entity_type) {
  return greyhead_encrypted_field_acl_entity_type_info($acl_entity_type) !== FALSE;
}

/**
 * Gets information on the types of ACL entities which can be assigned access.
 *
 * @TODO: check this documentation isn't out of date.
 *
 * The return value is an array whose keys are the entity type's ACL name, and
 * the value is an array containing:
 *
 *  - module: the name of the module providing the access check
 *
 *  - access callback: the function name which checks whether a user has this
 *    access
 *
 *  - validation callback: the function which checks that an ACL property ID is
 *    valid. This function is called when an encrypted field is submitted and
 *    ACLs are being saved.
 *
 * Callback functions must accept one argument, which is an options array with
 * the following values:
 *
 *  - property_id: the identifier of the property which the user account is to
 *    be checked for, for example a role ID, the user type "authenticated user",
 *    etc.
 *
 *  - uid (for access callbacks): the user ID being checked. Access callbacks
 *    should not assume that a NULL user ID means it should check the current
 *    global user entity.
 *
 *  - field_instance: the current instance of the field. This information is
 *    needed, for example, for checks which need to know the UID which created
 *    the field.
 *
 *  - entity (where applicable): the entity to which the field is attached.
 *
 * (&$item, $delta, &$field, $instance, &$entity)
 *
 * Access callback functions should return TRUE if the user explicitly DOES have
 * the property in property_id; FALSE should be returned if they do not.
 *
 * Validation callbacks should return TRUE if the submitted property_id is valid
 * or FALSE if not.
 */
function greyhead_encrypted_field_acl_entity_types_info() {
  $acl_entity_types = array();

  // Implement a hook_greyhead_encrypted_field_acl_entity_types_info_alter.
  drupal_alter(GEF_MODULE_NAME . '_acl_entity_types_info', $acl_entity_types);

  return $acl_entity_types;
}

/**
 * Implements hook_greyhead_encrypted_field_acl_entity_types_info_alter.
 *
 * @param $acl_entity_types
 */
function greyhead_encrypted_field_greyhead_encrypted_field_acl_entity_types_info_alter(&$acl_entity_types) {
  $acl_entity_types[GEF_USER_TYPE] = array(
    'module' => GEF_MODULE_NAME,
    'access callback' => GEF_MODULE_NAME . '_acl_access_user_type',
    'validation callback' => GEF_MODULE_NAME . '_acl_validate_user_type',
  );

  $acl_entity_types[GEF_USER_UID] = array(
    'module' => GEF_MODULE_NAME,
    'access callback' => GEF_MODULE_NAME . '_acl_access_user_uid',
    'validation callback' => GEF_MODULE_NAME . '_acl_validate_user_uid',
  );

  $acl_entity_types[GEF_USER_NAME] = array(
    'module' => GEF_MODULE_NAME,
    'access callback' => GEF_MODULE_NAME . '_acl_access_user_name',
    'validation callback' => GEF_MODULE_NAME . '_acl_validate_user_name',
  );

  $acl_entity_types[GEF_ROLE_DRUPAL] = array(
    'module' => GEF_MODULE_NAME,
    'access callback' => GEF_MODULE_NAME . '_acl_access_role_drupal',
    'validation callback' => GEF_MODULE_NAME . '_acl_validate_role_drupal',
  );

  $acl_entity_types[GEF_ROLE_CUSTOM] = array(
    'module' => GEF_MODULE_NAME,
    'access callback' => GEF_MODULE_NAME . '_acl_access_role_custom',
    'validation callback' => GEF_MODULE_NAME . '_acl_validate_role_custom',
  );
}

/**
 * Checks whether the current user has administrative control over fields.
 *
 * If the entity type is 'node' and they have 'bypass node access' or
 * 'administer content types' permissions, or if they have 'administer encrypted
 * fields' permission, then this function will return TRUE. Otherwise, it will
 * return FALSE.
 *
 * @param $account
 * @param $entity_type
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_user_is_admin($account, $entity_type = NULL) {
  // Is this a node?
  if (!is_null($entity_type) && ($entity_type == 'node')) {
    // First we'll check if the user has the 'superuser'
    // permissions that node provides. This way administrators
    // will be able to administer the content types.
    if (user_access('bypass node access', $account)) {
      return TRUE;
    }

    if (user_access('administer content types', $account)) {
      return TRUE;
    }
  }

  // Now check if the user has the 'access all encrypted fields' permission.
  if (user_access(GEF_PERMISSION_ACCESS_ALL, $account)) {
    return TRUE;
  }

  return FALSE;
}

/**
 * Gets a list of permissions which define access to encrypted fields.
 *
 * @param string $permission A valid permission name (not title), such as "edit
 *                           title".
 *
 * @return array Either an array of all permissions if $permission wasn't
 *               provided, or a permission array.
 */
function greyhead_encrypted_field_acl_permissions_available($permission = NULL) {
  $permissions = &drupal_static(__FUNCTION__);

  if (is_null($permissions)) {
    $permissions = array();

    // Implement a hook_greyhead_encrypted_field_acl_permissions_available_alter.
    drupal_alter(GEF_MODULE_NAME . '_acl_permissions_available', $permissions);
  }

  // Have we been asked for a specific permission? Validate it first.
  if (!is_null($permission)) {
    if (greyhead_encrypted_field_acl_valid_permission($permission, $permissions)) {
      return $permissions[$permission];
    }

    // If we get here, we've been passed a duff permission name.
    return FALSE;
  }

  return $permissions;
}

/**
 * Implements hook_greyhead_encrypted_field_acl_permissions_available_alter.
 *
 * @param $permissions
 */
function greyhead_encrypted_field_greyhead_encrypted_field_acl_permissions_available_alter(&$permissions) {
  $permissions[GEF_PERMISSION_FULL_CONTROL] = array(
    'title' => t('Full control'),
    'description' => t('This permission allows a user to view and edit an encrypted field and its title, and edit permissions on a field.'),
    'implicit permissions' => array(
      GEF_PERMISSION_EDIT_PERMISSIONS,
      GEF_PERMISSION_VIEW_PERMISSIONS,
      GEF_PERMISSION_EDIT_VALUE,
      GEF_PERMISSION_VIEW_VALUE,
      GEF_PERMISSION_EDIT_TITLE,
      GEF_PERMISSION_VIEW_TITLE,
    ),
    'also granted by' => array(),
  );

  $permissions[GEF_PERMISSION_EDIT_PERMISSIONS] = array(
    'title' => t('Edit an encrypted field\'s permissions'),
//    'description' => t(''),
    'implicit permissions' => array(
      GEF_PERMISSION_FULL_CONTROL,
    ),
    'also granted by' => array(
      GEF_PERMISSION_FULL_CONTROL,
    ),
  );

  $permissions[GEF_PERMISSION_VIEW_PERMISSIONS] = array(
    'title' => t('View an encrypted field\'s permissions'),
//    'description' => t(''),
    'implicit permissions' => array(),
    'also granted by' => array(
      GEF_PERMISSION_FULL_CONTROL,
      GEF_PERMISSION_EDIT_PERMISSIONS,
    ),
  );

  $permissions[GEF_PERMISSION_EDIT_VALUE] = array(
    'title' => t('Edit field value'),
//    'description' => t(''),
    'implicit permissions' => array(
      GEF_PERMISSION_VIEW_VALUE,
    ),
    'also granted by' => array(
      GEF_PERMISSION_FULL_CONTROL,
      GEF_PERMISSION_EDIT_PERMISSIONS,
    ),
  );

  $permissions[GEF_PERMISSION_VIEW_VALUE] = array(
    'title' => t('View field value'),
//    'description' => t(''),
    'implicit permissions' => array(),
    'also granted by' => array(
      GEF_PERMISSION_FULL_CONTROL,
      GEF_PERMISSION_EDIT_PERMISSIONS,
      GEF_PERMISSION_EDIT_VALUE,
    ),
  );

  $permissions[GEF_PERMISSION_EDIT_TITLE] = array(
    'title' => t('Edit field title'),
//    'description' => t(''),
    'implicit permissions' => array(
      GEF_PERMISSION_VIEW_TITLE,
    ),
    'also granted by' => array(
      GEF_PERMISSION_FULL_CONTROL,
      GEF_PERMISSION_EDIT_PERMISSIONS,
    ),
  );

  $permissions[GEF_PERMISSION_VIEW_TITLE] = array(
    'title' => t('View field title'),
//    'description' => t(''),
    'implicit permissions' => array(),
    'also granted by' => array(
      GEF_PERMISSION_FULL_CONTROL,
      GEF_PERMISSION_EDIT_PERMISSIONS,
      GEF_PERMISSION_EDIT_TITLE,
    ),
  );
}

/**
 * Access callback: does a given user ID match an ACL's UID entry?
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_access_user_uid(array $options) {
  return $options['property_id'] == $options['uid'];
}

/**
 * Validation callback which verifies that a given user ID exists.
 *
 * @param array $options
 *
 * @return bool TRUE if the user account exists and can be loaded; FALSE if not.
 */
function greyhead_encrypted_field_acl_validate_user_uid($options) {
  return (user_load($options['property_id']) !== FALSE);
}

/**
 * Access callback: does a given user's name match the username in the ACL?
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_access_user_name(array $options) {
  if ($user = user_load_by_name($options['property_id'])) {
    return (bool) $user->uid == $options['uid'];
  }

  return FALSE;
}

/**
 * Validation callback: does a given username exist?
 *
 * @param array $options
 *
 * @return bool TRUE if the user account exists and can be loaded; FALSE if not.
 */
function greyhead_encrypted_field_acl_validate_user_name(array $options) {
  return (user_load_by_name($options['property_id']) !== FALSE);
}

/**
 * Access callback: does a given user have a particular type of Drupal role?
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_access_role_drupal(array $options) {
  $return = FALSE;
  // What property_id do we need to test the user account for?
  switch ($options['property_id']) {
    // Find out if the user is a member of the current administraton role.
    case GEF_ROLE_DRUPAL_ADMINISTRATOR:
      $return = (bool) user_has_role(variable_get('user_admin_role'), user_load($options['uid']));
      break;

    // Find out if the user is authenticated.
    case GEF_ROLE_DRUPAL_AUTHENTICATED:
      $return = (bool) $options['uid'] > 0;
      break;

    // Find out if the user is anonymous.
    case GEF_ROLE_DRUPAL_ANONYMOUS:
      $return = (bool) ($options['uid'] == 0) || is_null($options['uid']);
      break;
  }

  return $return;
}

/**
 * Validation callback to validate that a Drupal built-in role name is valid.
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_validate_role_drupal(array $options) {
  return in_array($options['property_id'], array(
    GEF_ROLE_DRUPAL_ADMINISTRATOR,
    GEF_ROLE_DRUPAL_AUTHENTICATED,
    GEF_ROLE_DRUPAL_ANONYMOUS,
  ), TRUE);
}

/**
 * Access callback: is a given user ID a member of a particular role?
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_access_role_custom(array $options) {
  return (bool) user_has_role_by_name($options['property_id'], user_load($options['uid']));
}

/**
 * Check that the ACLs provided are valid.
 *
 * @param array $acls An array of ACL strings as the array key, with the value
 *                    being an array of data. This function will set the 'valid'
 *                    key to TRUE or FALSE for each ACL entry.
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_validate_acls(array &$acls) {
  // Start by assuming they're all valid until proven otherwise.
  $valid = TRUE;

  foreach ($acls as &$acl) {
    $valid = $valid && $acl->validate();
  }

  return $valid;
}

/**
 * Checks whether an array of permissions is valid.
 *
 * This function is designed to be used to validate the permissions array from
 * greyhead_encrypted_field_acl_explode().
 *
 * @param array $permissions
 *
 * @return bool TRUE if all permissions are recognised, or FALSE if one or more
 *              were unknown.
 */
function greyhead_encrypted_field_acl_validate_permissions(array $permissions) {
  // Get all the configured permissions available to ACLs.
  $available_permissions = greyhead_encrypted_field_acl_permissions_available();

  // Start on the assumption that they're all valid, but one invalid permission
  // will result in a false.
  $valid = TRUE;

  foreach ($permissions as $permission) {
    $permission_valid = greyhead_encrypted_field_acl_valid_permission($permission);

    if (!$permission_valid) {
      greyhead_encrypted_field_error('Unknown permission %permission in greyhead_encrypted_field_acl_validate_permissions() - available permissions: %permissions.', array(
        '%permission' => $permission,
        '%permissions' => implode(', ', $available_permissions),
      ));
    }

    $valid = $valid && $permission_valid;
  }

  return $valid;
}

/**
 * Check whether a single permission string is valid.
 *
 * Wrapper function for greyhead_encrypted_field_acl_validate_permissions().
 *
 * @param $permission
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_validate_permission($permission) {
  return greyhead_encrypted_field_acl_validate_permissions(array($permission));
}

/**
 * Validate that an entity is valid.
 *
 * @param $entity_type
 * @param $entity_id
 *
 * @return bool|mixed
 */
function greyhead_encrypted_field_acl_validate_entity($entity_type, $entity_id) {
  // Assume the entity is invalid until proven otherwise.
  $return_array = array(
    'valid_entity' => FALSE,
    'valid_entity_type' => FALSE,
    'valid_entity_id' => FALSE,
  );

  // Load the entity type info, which tells us the access callback.
  if ($acl_entity_type_info = greyhead_encrypted_field_acl_entity_type_info($entity_type)) {
    // It's a valid entity type.
    $return_array['valid_entity_type'] = TRUE;

    // Determine the validation function name.
    $validation_callback_function = $acl_entity_type_info['validation callback'];

    if (function_exists($validation_callback_function)) {
      // Assemble the $options array.
      $options = array(
        'property_id' => $entity_id,
//        'uid' => $user->uid,
//        'field_instance' => $instance,
        'entity_uid' => NULL,
        'entity' => NULL,
      );

      // At last, we know everything we need to - check if valid.
      $return_array['valid_entity_id'] = call_user_func($validation_callback_function, $options);

      if (!$return_array['valid_entity_id']) {
        greyhead_encrypted_field_error('Entity type %type valid but ID %id failed validation in function %function.', array(
          '%function' => $validation_callback_function,
          '%type' => $entity_type,
          '%id' => $entity_id,
        ));
      }
    }
    else {
      // Unknown validation function.
      greyhead_encrypted_field_error('Validation function %function not found for Entity type %type and ID %id.', array(
        '%function' => $validation_callback_function,
        '%type' => $entity_type,
        '%id' => $entity_id,
      ));
    }
  }
  else {
    // Unrecognised entity type.
    greyhead_encrypted_field_error('Unrecognised entity type %type with ID %id passed in greyhead_encrypted_field_acl_validate_entity().', array(
      '%type' => $entity_type,
      '%id' => $entity_id,
    ));
  }

  // Lastly, set $return_array['valid_entity'] to TRUE if both type and ID
  // validated.
  $return_array['valid_entity'] = $return_array['valid_entity_type']
    && $return_array['valid_entity_id'];

  return $return_array;
}

/**
 * Validation callback: is a given custom role name valid?
 *
 * Note that we don't use role IDs here, but instead the role names, for
 * portability.
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_validate_role_custom(array $options) {
  return (bool) get_role_by_name($options['property_id']);
}

/**
 * Checks if a user is of a particular type with respect to an encrypted field.
 *
 * For example, this function could be asked to check whether a user is the
 * original creator of a field.
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_access_user_type($options) {
  $return = FALSE;

  // What property_id do we need to test the user account for?
  switch ($options['property_id']) {
    // Find out if the user created the field.
    case GEF_USER_TYPE_CREATOR:
      $return = ($options['entity_uid'] == $options['uid']);
      break;

    // Find out if the user is the root user, i.e. UID 1.
    case GEF_USER_TYPE_ROOT:
      $return = (bool) $options['uid'] == 1;
      break;

    // Is the user part of the "everyone" group - always returns TRUE.
    case GEF_USER_TYPE_EVERYONE:
      $return = TRUE;
      break;
  }

  return $return;
}

/**
 * Checks whether the submitted user type is valid.
 *
 * @param array $options
 *
 * @return bool
 */
function greyhead_encrypted_field_acl_validate_user_type(array $options) {
  return in_array($options['property_id'], array(
    GEF_USER_TYPE_CREATOR,
    GEF_USER_TYPE_ROOT,
    GEF_USER_TYPE_EVERYONE,
  ), TRUE);
}

/**
 * Get the FAPI element for the title of the encrypted field.
 *
 * @param string    $default_value
 * @param bool|TRUE $required
 *
 * @return array
 */
function greyhead_encrypted_field_acl_get_label_element($default_value = '', $required = TRUE) {
  $widget_label_element = array(
    '#title' => t('Name of this encrypted field'),
    '#type' => GEF_FIELD_LABEL_FAPI_TYPE,
    '#required' => $required, // Only if on entity creation/edit form.
    '#default_value' => $default_value,
    '#maxlength' => 255,
    '#decrypted' => FALSE, // Set to TRUE if the data is decrypted.
    '#element_validate' => array(
      'greyhead_encrypted_field_encrypt',
    ),
  );

  return $widget_label_element;
}

/**
 * Get the FAPI element for the value of the encrypted field.
 *
 * @param string $default_value
 *
 * @return array
 */
function greyhead_encrypted_field_acl_get_value_element($widget_type, $default_value = '') {
  // Figure out which widget we need to generate.
  // In our case, there's only one type.
  $widget_value_element = array(
    '#title' => t('Data to encrypt'),
    '#default_value' => $default_value,
    '#type' => GEF_FIELD_VALUE_FAPI_TYPE,
    '#attributes' => array('class' => array('greyhead-encrypted-field')),
    '#decrypted' => FALSE, // Set to TRUE if the data is decrypted.
    '#element_validate' => array(
      'greyhead_encrypted_field_encrypt',
    ),
  );

  // Determine the widget type.
  switch ($widget_type) {
    case GEF_FIELD_WIDGET:
    default:
      $widget_value_element['#type'] = GEF_FIELD_VALUE_FAPI_TYPE;
      break;
  }

  return $widget_value_element;
}

/**
 * Gets a textarea element with any ACLs for the encrypted field instance.
 *
 * This function does not check access to the element.
 * @see      greyhead_encrypted_field_acl_check_widget_form_elements_access() for that.
 *
 * @param array $acls
 *
 * @return array
 */
function greyhead_encrypted_field_acl_get_acls_element($acls = array(), $entity = NULL, $sitewide_defaults_form = FALSE) {
  // Crufty data handling; if the ACLs aren't already in strings, convert now.
  if (!is_string($acls)) {
    $acls = greyhead_encrypted_field_acl_acls_to_strings($acls);
  }

  // Set some of the text according to the type of form.
  $intro_text = 'Set the access control lists for this encrypted field.';

  // Is this ACL field being shown on an entity - e.g. a node form - or the
  // field configuration form?
  if (is_null($entity)) {
    $intro_text = 'Set the default access control lists to use when creating new entities which contain this field.';
  }

  // Is this ACL field being shown on the module's overall config page.
  if ($sitewide_defaults_form) {
    $intro_text = 'Set the default access control lists to use when creating new fields.';
  }

  // Create the ACL textarea.
  $widget_acls_element = array(
    '#type' => 'textarea',
    '#title' => t('Permissions for this encrypted field'),
    '#description' => t('<p>!intro_text</p> '
      . '<p>ACLs are written in the form <em>[entity type]:[entity ID]::permissions:[permission 1],[permission 2],[...],[permission n]</em></p> '
      . '<p>For example, this ACL allows user monkeybones to edit the field and its label: <em>!entitytypeuser:monkeybones::permissions:!edittitle,!editvalue</em></p> '
      . '<p>The entity types available are: <em>!entitytypes</em>.</p> '
      . '<p>The permissions available are: <em>!permissionsavailable</em>.</p> '
      . '<p>By default, the user who created the field, '
      . 'and the site administrators, have full control over the field. Logged-'
      . 'in users can view the field title but can\'t view the encrypted data, '
      . 'and anonymous users cannot see anything.</p>', array(
      '!intro_text' => t($intro_text),
      '!entitytypeuser' => GEF_USER_NAME,
      '!edittitle' => GEF_PERMISSION_EDIT_TITLE,
      '!editvalue' => GEF_PERMISSION_EDIT_VALUE,
      '!entitytypes' => implode(', ', array_keys(greyhead_encrypted_field_acl_entity_types_info())),
      '!permissionsavailable' => implode(', ', array_keys(greyhead_encrypted_field_acl_permissions_available())),
    )),
    '#default_value' => $acls,
    '#decrypted' => TRUE, // This field is always decrypted.
    '#element_validate' => array(
      'greyhead_encrypted_field_acl_get_acls_element_validate',
      'greyhead_encrypted_field_encrypt',
    ),
  );

  return $widget_acls_element;
}

/**
 * Validation function which validates the ACLs submitted in an ACLs field.
 *
 * @param $element
 * @param $form_state
 */
function greyhead_encrypted_field_acl_get_acls_element_validate($element, &$form_state) {
  $acls_textarea_value = drupal_array_get_nested_value($form_state['values'], $element['#parents']);

  $acls = greyhead_encrypted_field_acl_textfield_to_acls($acls_textarea_value);

  // If one or more ACLs are invalid, create a list of invalid ACL strings.
  $acls_valid = greyhead_encrypted_field_acl_validate_acls($acls);
  if (!$acls_valid) {
    $invalid_acls = array();

    foreach ($acls as $acl_string => $acl) {
      if (!$acl->valid) {
        gefdpm($acl);
        $invalid_acl_parts = array();

        if (!$acl->valid_entity) {
          if (!$acl->valid_entity_type) {
            $invalid_acl_parts[] = 'entity type';
          }
          if (!$acl->valid_entity_id) {
            $invalid_acl_parts[] = 'entity ID';
          }
        }

        if (!$acl->valid_permissions) {
          $invalid_acl_parts[] = 'permissions';
        }

        $options = array(
          '!acl' => $acl_string,
          '!invalid_parts' => implode(', ', $invalid_acl_parts),
        );

        $invalid_acls[] = t('ACL "!acl" invalid parts: !invalid_parts', $options);
      }
    }

    form_error($element, t('<p>The following ACLs contain errors:</p> <ul><li>' . implode('</li> <li>', $invalid_acls) . '</li></ul>'));
  }
  else {
    // It's valid. Change the ACLs into an array of ACL objects for storing.
//    form_set_value($element, serialize($acls), $form_state);
  }

//  gefdpm(__FUNCTION__);
//  gefdpm($form_state);
//  gefdpm($acls_textarea_value);
//  gefdpm($acls);
}

/**
 * Re-encrypt submitted form data if it needs to be encrypted before saving.
 *
 * @param $element
 * @param $form_state
 */
function greyhead_encrypted_field_encrypt($element, &$form_state) {
  // Get the #decrypted boolean; if it's FALSE, we don't need to do anything;
  // if it's TRUE, we need to re-encrypt the data.
  $decrypted = $element['#decrypted'];

  if ($decrypted) {
    // We need to encrypt the element's value.
    // Get the element's value.
    $element_value = drupal_array_get_nested_value($form_state['values'], $element['#parents']);

    // Encrypt the value.
    $element_value_encrypted = encrypt($element_value);

    // Set the value back into $form_state['values'].
    form_set_value($element, $element_value_encrypted, $form_state);
  }

  gefdpm(__FUNCTION__);
  gefdpm($element);
  gefdpm($form_state['values']);

//  if ($element['#name'] == 'field_secret_data_3[und][0][acls]') {
//    gdpm($element);
//    gdpm($element_value);
//    gdpm($element_value_encrypted);
//    gdpm($form_state['values']);
//    die('greyhead_encrypted_field_encrypt');
//  }
}

/**
 * Determine the ACLs for an encrypted field.
 *
 * This function starts by checking whether $items[$items_delta] contains field
 * information, and uses the ACLs set in that field if it does.
 *
 * If $items[$items_delta] isn't set, the field is probably new, so we look in
 * $instance[GEF_SETTINGS_KEY][GEF_ACLS_KEY] for defaults and use those, if available.
 *
 * Failing that, we look at $field[GEF_SETTINGS_KEY][GEF_ACLS_KEY] for defaults,
 * which come from greyhead_encrypted_field_acl_default_acls().
 *
 * @param $field
 * @param $instance
 * @param $items
 * @param $items_delta
 *
 * @return array|null
 */
function greyhead_encrypted_field_acl_get_acls($field = NULL, $instance = NULL, $items = NULL, $items_delta = NULL) {
  if (isset($items, $items_delta, $items[$items_delta], $items[$items_delta][GEF_FIELD_ACLS_FAPI_KEY])
    && !empty($items[$items_delta][GEF_FIELD_ACLS_FAPI_KEY])
  ) {
    $acls = $items[$items_delta][GEF_FIELD_ACLS_FAPI_KEY];
  }
  elseif (isset($instance, $instance[GEF_SETTINGS_KEY][GEF_FIELD_DEFAULT_ACLS_FAPI_KEY])
    && !empty($instance[GEF_SETTINGS_KEY][GEF_FIELD_DEFAULT_ACLS_FAPI_KEY])
  ) {
    $acls = $instance[GEF_SETTINGS_KEY][GEF_FIELD_DEFAULT_ACLS_FAPI_KEY];
  }
  elseif (isset($field, $field[GEF_SETTINGS_KEY][GEF_FIELD_DEFAULT_ACLS_FAPI_KEY])
    && !empty($field[GEF_SETTINGS_KEY][GEF_FIELD_DEFAULT_ACLS_FAPI_KEY])
  ) {
    $acls = $field[GEF_SETTINGS_KEY][GEF_FIELD_DEFAULT_ACLS_FAPI_KEY];
  }
  else {
    $acls = greyhead_encrypted_field_acl_default_acls();
  }

  return $acls;
}

/**
 * Checks whether a user has access to view or edit an encrypted field's title,
 * value and ACLs.
 *
 * Depending on their access to each of the three parts, the user will either be
 * able to edit, view but not make changes, or they may not see anything.
 *
 * @see also greyhead_encrypted_field_acl_check_widget_form_element_access().
 *
 * @param      $widget_form
 * @param      $entity
 * @param null $account
 */
function greyhead_encrypted_field_acl_check_widget_form_elements_access(&$widget_form, $entity, $acls, $account = NULL) {
  // Check access to the label field.
  greyhead_encrypted_field_acl_check_widget_form_element_access($widget_form, GEF_FIELD_LABEL_FAPI_KEY, $entity, GEF_PERMISSION_EDIT_TITLE, GEF_PERMISSION_VIEW_TITLE, $acls, $account);

  // Check access to the value field.
  greyhead_encrypted_field_acl_check_widget_form_element_access($widget_form, GEF_FIELD_VALUE_FAPI_KEY, $entity, GEF_PERMISSION_EDIT_VALUE, GEF_PERMISSION_VIEW_VALUE, $acls, $account);

  // Check access to the ACLs field.
  greyhead_encrypted_field_acl_check_widget_form_element_access($widget_form, GEF_FIELD_ACLS_FAPI_KEY, $entity, GEF_PERMISSION_EDIT_PERMISSIONS, GEF_PERMISSION_VIEW_PERMISSIONS, $acls, $account);
}

/**
 * Checks whether a user has access to view or edit an encrypted field.
 *
 * Depending on their access, the user will either be able to edit the field's
 * value, view the field but not make changes, or see a message telling them
 * that the field is not accessible.
 *
 * @see also greyhead_encrypted_field_acl_check_widget_form_elements_access().
 *
 * @param      $widget_form
 * @param      $form_element_to_check
 * @param      $entity
 * @param      $edit_permission
 * @param      $view_permission
 * @param null $account
 */
function greyhead_encrypted_field_acl_check_widget_form_element_access(&$widget_form, $form_element_to_check, $entity, $edit_permission, $view_permission, $acls, $account = NULL) {
  if (is_null($account)) {
    $account = $GLOBALS['user'];
  }

  gefdpm(__FUNCTION__);
  gefdpm($widget_form, __FUNCTION__ . ' $widget_form');

  gefdpm('@TODO: pass in the entity so we can try to get the owner.');

  // Do they have view access?
  if (greyhead_encrypted_field_acl_user_access($view_permission, $acls, $entity, GEF_FIELD_TYPE, $account)) {
    // Yes. If the #decrypted flag is FALSE, we need to decrypt it and update
    // the flag.
    if (!$widget_form[$form_element_to_check]['#decrypted']) {
      $default_value = $widget_form[$form_element_to_check]['#default_value'];

      // If the value of this field is serialized, it's probably encrypted.
      if (@unserialize($default_value)) {
        $widget_form[$form_element_to_check]['#default_value'] = decrypt($default_value);
      }

      $widget_form[$form_element_to_check]['#decrypted'] = TRUE;
    }

    // Does the user have access to edit the field?
    if (!greyhead_encrypted_field_acl_user_access($edit_permission, $acls, $entity, GEF_FIELD_TYPE, $account)) {
      // No. Disable the field but allow them to view its decrypted value.
      $widget_form[$form_element_to_check]['#disabled'] = TRUE;
      $widget_form[$form_element_to_check]['#value'] = $widget_form[$form_element_to_check]['#default_value'];
    }
  }
  else {
    // No access at all. Hide the field's contents by copying the label field,
    // hiding the existing label field, and changing the copied field's text.
    $widget_form[$form_element_to_check . '_copy'] = $widget_form[$form_element_to_check];
    $widget_form[$form_element_to_check]['#type'] = 'value';

    // Work out the type of element so we can show a sensible "no access"
    // message.
    $thing = 'field';

    if ($form_element_to_check == GEF_FIELD_LABEL_FAPI_KEY) {
      $thing = 'label';
    }
//    elseif ($form_element_to_check == GEF_FIELD_VALUE_FAPI_KEY) {
//      $thing = 'data';
//    }

    $widget_form[$form_element_to_check . '_copy']['#value'] = t(GEF_NO_ACCESS, array('!thing' => t($thing)));
  }
}

/**
 * Log an error.
 *
 * @param     $message
 * @param     $parameters
 * @param int $severity
 */
function greyhead_encrypted_field_error($message, $parameters, $severity = WATCHDOG_ERROR, $database_only = FALSE) {
  if (!$database_only) {
    drupal_set_message(t($message, $parameters), 'error', FALSE);
  }

  watchdog(GEF_MODULE_TITLE, $message, $parameters, $severity);
}

/**
 * Wrapper around gefdpm() to allow us to turn debugging output on and off.
 *
 * @param      $variable
 * @param null $context
 */
function gefdpm($variable, $context = NULL) {
  if (variable_get('greyhead_encrypted_field_debug')
    && function_exists('dpm')
  ) {
    if (!is_null($context)) {
      gefdpm($context);
    }

    gefdpm($variable);
  }
}
