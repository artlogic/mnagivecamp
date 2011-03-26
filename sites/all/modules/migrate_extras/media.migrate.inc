<?php
// $Id: media.migrate.inc,v 1.1.2.8 2011/01/28 19:57:25 mikeryan Exp $

/**
 * @file
 * Support for media destinations.
 */

/**
 * Destination class implementing migration into media entities.
 */
class MigrateDestinationMedia extends MigrateDestinationFile {
  /**
   * List of node migrations, whose bodies need image references fixed up.
   *
   * @var array
   */
  protected $sourceMigrations = array();

  /**
   * Node ID of the node whose body is currently being rewritten.
   *
   * @var int
   */
  protected $parentNid;

  /**
   * Return an options array for media destinations.
   *
   * @param array $source_migrations
   *  List of node migrations, whose bodies need image references fixed up.
   * @param boolean $copy_file
   *  TRUE to have the file copied from the provided URI to Drupal. Defaults to FALSE.
   * @param string $copy_destination
   *  If $copy_file is TRUE, the scheme/directory to use as the destination for the copied file.
   *  Defaults to 'public://'.
   * @param string $language
   *  Default language for files created via this destination class.
   * @param string $text_format
   *  Default text format for files created via this destination class.
   */
  static public function options($source_migrations, $copy_file, $copy_destination, $language, $text_format) {
    return compact('source_migrations', 'copy_file', 'copy_destination', 'language', 'text_format');
  }

  /**
   * Basic initialization
   *
   * @param array $options
   *  Options applied to files.
   */
  public function __construct($media_type = 'default', array $options = array()) {
    if (isset($options['source_migrations'])) {
      $this->sourceMigrations = $options['source_migrations'];
    }
    parent::__construct($options);
    $this->entityType = 'media';
    $this->bundle = $media_type;
  }

  /**
   * Returns a list of fields available to be mapped for the entity type (bundle)
   *
   * @return array
   *  Keys: machine names of the fields (to be passed to addFieldMapping)
   *  Values: Human-friendly descriptions of the fields.
   */
  public function fields() {
    // First get the file fields
    $fields = parent::fields();

    $fields += migrate_handler_invoke_all('Media', 'fields');

    return $fields;
  }

  /**
   * The file import method calls file_save(), which invokes media hooks but does
   * not call media_save(), thus fields are not saved. So, we do the media_save()
   * ourselves after file_save().
   *
   * @param $file
   *  File object to build. This is the complete object after saving.
   * @param $source_row
   *  Raw source data object - passed through to complete handlers.
   */
  public function complete(stdClass $file, stdClass $source_row) {
    parent::complete($file, $source_row);
    media_save($file);
  }

  /**
   * Called at the completion of a migration into a media destination. Rewrite
   * <img> tags into the media module's embedded JSON references.
   */
  public function postImport() {
    migrate_instrument_start('MigrateDestinationMedia postImport');
    // Scan the destination nodes
    foreach ($this->sourceMigrations as $source_migration) {
      $migration = MigrationBase::getInstance($source_migration);
      $map = $migration->getMap();
      foreach ($map as $map_row) {
        $this->parentNid = $map_row->destid1;
        $node = node_load($this->parentNid);
        $body = $node->body[LANGUAGE_NONE][0]['value'];
        // Quickly skip any non-candidates
        if (!strstr($body, 'img')) {
          continue;
        }
        // Pass full img tags into the callback
        $new_body = preg_replace_callback('|<img [^>]*>|i', array($this, 'replaceCallback'),
          $body);
        $node->body[LANGUAGE_NONE][0]['value'] = $new_body;
        node_save($node);
      }
    }
    migrate_instrument_stop('MigrateDestinationMedia postImport');
  }

  /**
   * If a referenced image can be found in the files table, replace the <img> tag
   * with a media JSON reference.
   *
   * @param array $matches
   */
  protected function replaceCallback(array $matches) {
    $src_matches = array();
    // Default to the original <img> tag
    $result = $matches[0];
    // Extract the src parameter
    if (preg_match('|src=[\'"]([^\'"]+)[\'"]|i', $matches[0], $src_matches)) {
      // Replace the scheme and host portions of the referenced URL with the
      // Drupal scheme as it's saved in the file_unmanaged table
      $src = preg_replace('|^https?://[^/]+/|', $this->copyDestination, $src_matches[1]);
      $file = db_select('file_managed', 'f')
              ->fields('f', array('fid'))
              ->condition('filename', basename($src))
              ->execute()
              ->fetchObject();
      if ($file) {
        // Update file_usage
        // Note: Usages not cleaned up on file_delete() - NYI in media module
        file_usage_add($file, 'media', 'node', $this->parentNid);

        // TODO: Anything to do about alt/title?
        $image_info = array(
          'type' => 'media',
          'view_mode' => 'media_large',
          'fid' => $file->fid,
          'attributes' => array(
            'alt' => '',
            'title' => '',
            'class' => 'media-image',
            'typeof' => 'foaf:Image',
            'wysiwyg' => 1,
          ),
        );
        $result = '[[' . drupal_json_encode($image_info) . ']]';
      }
    }
    return $result;
  }
}
