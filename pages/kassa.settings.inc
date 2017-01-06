<?php:

function kassa_settings_form($form,$form_state){
	$kassas = array();
	$q = db_select('kassa_kassa');
	$q->fields('kassa_kassa');
	$result = $q->execute()->fetchAll();	
	
	foreach($result as $kassa){
		$kassas[$kassa->kassa_id] = $kassa;
	}
	 if (isset($form_state['input']['kassas'])) {
    $keys = array(1 => 1) + drupal_map_assoc(array_keys($form_state['values']['kassas']['tabel']));

    $trace = $form_state['triggering_element']['#parents'];

    if ($trace[0] == 'kassas' && $trace[1] == 'tabel_add') {
      $i = 1;
      while (isset($keys['new:' . $i])) {
        $i++;
      }
      $keys['new:' . $i] = 'new:' . $i;
    }
    elseif ($trace[0] == 'kassas' && $trace[1] == 'tabel' && $trace[3] == 'action') {
      $key = $trace[2];
      unset($keys[$key]);
    }
  }
  else {
    $keys = drupal_map_assoc(array_keys($kassas));
  }

  // Show table
  $form['kassas'] = array(
      '#title' => t('Cash decks'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#tree' => TRUE,
      );

  $form['kassas']['tabel'] = array(
      '#type' => 'formtable',
      '#header' => array(
        t('ID'),
        t('Cash deck'),
        t('Action'),
        ),
      '#prefix' => '<div id="kassas">',
      );

  // Table
  foreach ($keys as $key) {
    if (isset($kassas[$key])){
      $kassa = $kassas[$key];

      $form['kassas']['tabel'][$key]['id'] = array(
          '#type' => 'item',
          '#markup' => '#' . $kassa->kassa_id,
          );
      if ($kassa->kassa_id == 1) {
        $form['kassas']['tabel'][$key]['naam'] = array(
            '#type' => 'item',
            '#markup' => $kassa->kassa,
            );
      }else {
      $form['kassas']['tabel'][$key]['naam'] = array(
          '#type' => 'textfield',
          '#default_value' => $kassa->kassa,
          '#maxlength' => 20,
          '#required' => TRUE,
          );
    	}
      }else{
	$form['kassas']['tabel'][$key]['id'] =array(
	'#type' => 'item',
	'#markup' => '',
	);
	$form['kassas']['tabel'][$key]['naam'] = array(
	'#type' => 'textfield',
	'#default_value' => '',
	'#maxlength' => 20,
	'#required' => true,
	);
      }


    $validate = array();
    foreach ($keys as $k) {
      if ($k != $key) {
        $validate[] = array('kassas', 'tabel', $k);
      }
    }
    $validate[] = array('kassas', 'tabel_add');

    $form['kassas']['tabel'][$key]['action'] = array(
        '#type' => 'button',
        '#value' => t('Remove'),
        '#attributes' => array(
          'style' => 'margin: 0;',
          ),
//        '#access' =>$kassa->kassa_id !=1 && db_query('SELECT COUNT(*) FROM {kassa_verkoop_bevat} WHERE kassa_id = :id', array(':id' => $kassa->kassa_id))->fetchField() == 0,
        '#ajax' => array(
          'callback' => 'kassa_settings_ajax_kassas',
          'wrapper' => 'kassas',
          ),
        '#name' => 'leveranciers_' . $key,
	'#limit_validation_errors' => $validate,
        '#validate' => array(),
        );
  }

  $form['kassas']['tabel_add'] = array(
      '#type' => 'button',
      '#value' => t('Add new line'),
      '#ajax' => array(
        'callback' => 'kassa_settings_ajax_kassas',
        'wrapper' => 'kassas',
        ),
      '#name' => 'leveranciers_new',
      '#validate' => array(),
      '#suffix' => '</div>',
      );

global $base_url;
if($base_url != 'www.wina.be'){

	$form['scientica'] = array(
	'#type' => 'fieldset',
	'#title' => t('Settings Scientica'),
	'#collapsible' => true,
	);

	$form['scientica']['settings'] = array(
	'#type' => 'formtable',
	'#title' => t('Settings Associations'),
	'#header' => array(
		t('Association'),
		t('Amount of Students'),
		),
	'#tree' => true,
	);
	foreach(kassa_get_kringen() as $kring){	
		$form['scientica']['settings'][$kring] = array(
		'kring' => array(
			'#type' => 'item',
			'#markup' => $kring,
			),
		'aantal' => array(
			'#type' => 'textfield',
			'#title' => t('Number of Students from') . $kring,
			'#default_value' => variable_get($kring,0),
			),
		);
	}
	
}
  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
      );
  return $form;
}

function kassa_settings_ajax_kassas(&$form,&$form_state){
	return array_intersect_key($form['kassas'], drupal_map_assoc(element_children($form['kassas'])));
}

function kassa_settings_form_submit(&$form,&$form_state){

	foreach($form_state['values']['settings'] as $key => $val){
		variable_set($key,$val['aantal']);
   	}
   $transaction = db_transaction();

  // Huidige kassas
  $kassas = array();
  foreach (db_query('SELECT * FROM {kassa_kassa} ORDER BY kassa_id = 1 DESC,kassa') as $kassa) {
    $kassas[$kassa->kassa_id] = $kassa;
  }

  // Test
  if (!isset($kassas[1])) {
    drupal_set_message(t('The default cash deck is missing!'), 'error');
    return $form;
  }

  // Delete oude types
  $delete = array_diff_key($kassas, $form_state['values']['kassas']['tabel']);
  if ($delete) {
    db_delete('kassa_kassa')
      ->condition('kassa_id', array_keys($delete))
      ->condition('kassa_id', 1, '!=')
      ->execute();
  }

  // Insert nieuwe types, update bestaande types
  if (isset($form_state['values']['kassas']['tabel'])) {
    foreach ($form_state['values']['kassas']['tabel'] as $key => $row) {
      if (substr($key, 0, 4) == 'new:') {
        db_insert('kassa_kassa')
          ->fields(array(
                'kassa' => $row['naam'],
                ))
          ->execute();
      }
      elseif ($key != 1) {
        db_update('kassa_kassa')
          ->fields(array(
                'kassa' => $row['naam'],
                ))
          ->condition('kassa_id', $key)
          ->execute();
      }
    }
  }

}
