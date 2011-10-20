<?php
// $Id: pathauto.migrate.inc,v 1.1.2.1 2010/10/04 19:11:06 mikeryan Exp $

/**
 * @file
 * Support for the Pathauto module.
 */

/**
 * Field handler.
 */
class MigratePathautoHandler extends MigrateDestinationHandler {
  public function __construct() {
    $this->registerTypes(array('node', 'user', 'taxonomy_term'));
  }

  /**
   * Make the destination field visible.
   */
  public function fields() {
    return array(
      'pathauto_perform_alias' =>
        t('Pathauto: Perform aliasing (set to 0 to prevent alias generation during migration'),
    );
  }
}
