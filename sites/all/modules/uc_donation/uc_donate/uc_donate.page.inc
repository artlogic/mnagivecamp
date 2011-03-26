<?php

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function uc_donate_form($form, &$form_state) {
  $form = array('items' => array());
  $donation = array(
    'nid' => array(
      '#type' => 'value',
      '#value' => ''
    ),
    'image' => array(
      '#type' => 'item',
      '#value' => ''
    ),
    'title' => array(
      '#type' => 'item',
      '#value' => ''
    ),
    'description' => array(
      '#type' => 'item',
      '#value' => ''
    ),
    'donate_amount' => array(
      '#type' => 'textfield',
      '#title' => t('Amount'),
      '#default_value' => '0.00',
      '#size' => 10,
      '#field_prefix' => variable_get('uc_sign_after_amount', FALSE) ? '' : variable_get('uc_currency_sign', '$'),
      '#field_suffix' => variable_get('uc_sign_after_amount', FALSE) ? variable_get('uc_currency_sign', '$') : ''
    ),
    'status' => array(
      '#type' => 'hidden',
      '#value' => ''
    ),
    '#theme' => 'uc_donate_donation_item',
    '#tree' => TRUE
  );
  if ($items = uc_donate_load()) {
    foreach ($items as $item) {
      switch ($item['type']) {
        case 'group':
          $id = 'group-' . $item['donate_id'];
          $form['items'][$id] = array();
          $form['items'][$id]['title'] = array('#type' => 'value', '#value' => $item['title']);
          $form['items'][$id]['#theme'] = 'uc_donate_group';
          break;
        case 'donation':
          $node = node_load($item['nid']);
          $donation['nid']['#value'] = $node->nid;
          // TODO Please change this theme call to use an associative array for the $variables parameter.
          $donation['image']['#value'] = isset($node->field_image_cache[0]) ? theme('imagecache', 'product_list', $node->field_image_cache[0]['filepath'], $node->field_image_cache[0]['alt'], $node->field_image_cache[0]['title']) : '';
          $donation['title']['#value'] = $node->title;
          $donation['description']['#value'] = $node->body;
          $donation['status']['#value'] = $node->status;
          if ($item['pid'] == 0) {
            if (isset($donation['group'])) {
              unset($donation['group']);
            }
            $id = 'donate-' . $item['donate_id'];
            if (node_access('view', $node)) {
              $form['items'][$id] = $donation;
            }
          }
          else {
            $donation['group'] = array('#type' => 'hidden', '#value' => $id);
            if (node_access('view', $node)) {
              $form['items'][$id]['donate-' . $item['donate_id']] = $donation;
            }
          }
          break;
      }
    }
  }

  $form['buttons']['add'] = array(
    '#type' => 'submit',
    '#value' => variable_get('uc_donate_add', t('Add to Cart'))
  );
  if (variable_get('uc_checkout_enabled', TRUE)) {
    $form['buttons']['checkout'] = array(
      '#type' => 'submit',
      '#value' => variable_get('uc_donate_checkout', t('Checkout'))
    );
  }
  return $form;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function uc_donate_form_validate($form, &$form_state) {
  foreach ($form_state['values'] as $key => $value) {
    if (substr($key, 0, 6) == 'donate') {
      $donate_amount = floatval(ereg_replace('[^0-9\.]*', '', $form_state['values'][$key]['donate_amount']));
      if (!is_numeric($donate_amount) || $donate_amount < 0) {
        form_set_error($key, t('Amount value has to be a number and can not be less than 0.'));
      }
      else {
        $form_item = (isset($value['group'])) ? $form['items'][$value['group']][$key]['donate_amount'] : $form['items'][$key]['donate_amount'];
        form_set_value($form_item, $donate_amount, $form_state);
      }
    }
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function uc_donate_form_submit($form, &$form_state) {
  foreach ($form_state['values'] as $key => $value) {
    if (substr($key, 0, 6) == 'donate') {
      if ($value['donate_amount'] > 0) {
        $nid = $value['nid'];
        $form_values = array(
          'nid' => $nid,
          'qty' => 1,
          'donate_amount' => $value['donate_amount']
        );
        uc_cart_add_item($nid, 1, module_invoke_all('add_to_cart_data', $form_values));
      }
    }
  }

  if ($form_state['values']['op'] == $form_state['values']['add']) {
    $form_state['redirect'] = 'cart';
  }
  else {
    $form_state['redirect'] = 'cart/checkout';
  }
}

/*******************************************************************************
 * Theme Functions
 ******************************************************************************/

/**
 * Theme a donation item for the donate page.
 */
function theme_uc_donate_donation_item($variables) {
  $form = $variables['form'];
  $form['title']['#printed'] = TRUE;
  $class = ($form['status']['#value']) ? 'donate-item' : 'donate-item node-unpublished';
  $image = ($form['image']['#value']) ? '<div class="donate-image">' . drupal_render($form['image']) . '</div>' : '';
  $form['image']['#printed'] = TRUE;
  $class .= ($image) ? ' image' : '';
  $output = '<div class="' . $class . '">';
  $output .= $image;
  $output .= '<div class="donate-info">';
  $output .= '<div class="donate-info-squeeze">';
  $output .= '<div class="donate-title"><h4>' . l($form['title']['#value'], 'node/' . $form['nid']['#value']) . '</h4></div>';
  $output .= '<div class="donate-description">' . drupal_render($form['description']) . '</div>';
  $output .= '</div>';
  $output .= '</div>';
  $output .= '<div class="donate-amount">' . drupal_render($form['donate_amount']) . '</div>';
  $output .= drupal_render_children($form);
  $output .= '<div class="clear-block"></div>';
  $output .= '</div>';
  return $output;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_uc_donate_group($variables) {
  $form = $variables['form'];
  $items = element_children($form);
  $output = '';
  $count = count($items) - 1;
  if ($count > 0 || user_access('administer donate')) {
    $form['title']['#printed'] = TRUE;
    $output .= '<div class="donate-group">';
    $empty_group_message = variable_get('uc_donate_empty_group_message', t('No donation products have been added to this group.'));
    if ($count < 1 && $empty_group_message != '') {
      // Make note for admins that this group does not currently contain any donation items.
      $output .= '<h3>' . check_plain($form['title']['#value']) . ' <span>(Empty group only visible by admins)</span></h3>';
      $output .= $empty_group_message;
    }
    else {
      $output .= '<h3>' . check_plain($form['title']['#value']) . '</h3>';
    }
    $output .= drupal_render_children($form);
    $output .= '</div>';
  }
  else {
    // Mark group as printed if no donation items exist in group
    $form['#printed'] = TRUE;
  }
  return $output;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_uc_donate_form($variables) {
  $form = $variables['form'];
  $items = drupal_render($form['items']);
  if (!$items) {
    if (user_access('administer donate')) {
      return '<p>' . t('No donation products have been added to the donate page. !add.', array('!add' => l(t('Add a donation'), 'admin/store/settings/donate/manage/add'))) . '</p>';
    }
    else {
      drupal_not_found();
      exit();
    }
  }

  drupal_add_css(drupal_get_path('module', 'uc_donate') . '/uc_donate.css');
  drupal_add_js(drupal_get_path('module', 'uc_donate') . '/jquery.highlightFade.js');
  drupal_add_js(drupal_get_path('module', 'uc_donate') . '/uc_donate.js');

  if (variable_get('uc_sign_after_amount', FALSE)) {
    $span = '<span></span>' . variable_get('uc_currency_sign', '$');
  }
  else {
    $span = variable_get('uc_currency_sign', '$') . '<span></span>';
  }

  $output = '<div id="donate-page">';
  $output .= '<div id="donate-items">' . $items . '</div>';
  $output .= '<h4 id="donate-total">Total: ' . $span . '</h4>';
  $output .= '<div id="donate-buttons">' . drupal_render($form['buttons']) . '</div>';
  $output .= '<div class="clear-block">' . drupal_render_children($form) . '</div>';
  $output .= '</div>';
  return $output;
}

